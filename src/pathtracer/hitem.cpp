#include"hitem.h"
#include"bsdf.h"

/*-----------------------------------------------------------*/
/*--------------------IntersectRecord------------------------*/
/*-----------------------------------------------------------*/

IntersectRecord& IntersectRecord::operator=(const IntersectRecord& inst){
    if(this==&inst)
        return *this;

    pos_=inst.pos_;
    t_=inst.t_;
    normal_=inst.normal_;
    material_=inst.material_;
    uv_=inst.uv_;
    
    bvhnode_idx_=inst.bvhnode_idx_;

    return *this;
}

// use material to initialize bsdf
std::shared_ptr<BSDF> IntersectRecord::getBSDF(float u){

    // init TBN Matrix
    if(!TBN_)  TBN_=genTBN();
    assert(material_);

    auto type=this->material_->type_;
    // set emission bits to 0
    type=MtlType((int)type&(~(int)MtlType::Emissive));

    auto bsdf_=std::make_shared<BSDFlist>();

    // test scattering bits
    bool init=false;
    if((int)(type&MtlType::Diffuse)){
        if(material_->dif_texture_){
            auto ks= utils::srgbToLinear( material_->getDiffuse(uv_[0],uv_[1]) );
            bsdf_->insertBSDF(std::make_shared<LambertBRDF>(ks));
        }
        else
            bsdf_->insertBSDF(std::make_shared<LambertBRDF>(material_->diffuse_));

        init=true;
    }
    if((int)(type&MtlType::Specular)){
        if(!init)// TODO: delete this when xml-parser is prepared
            bsdf_->insertBSDF(std::make_shared<SpecularBRDF>(material_->getSpecular()));
        else
            bsdf_->insertBSDF(std::make_shared<BPhongSpecularBRDF>(material_->getSpecular(),material_->shininess_));

        init=true;
    }

    // If the scattering type is not initialized, simply give it a LambertBRDF
    if(!init){
        bsdf_->insertBSDF(std::make_shared<LambertBRDF>(material_->diffuse_));
    }

    // Init bsdf weights and choose a bsdf
    bsdf_->initWeights();
    bsdf_->randomSelectBSDF(u);
    return bsdf_;
}

// Generate an orthonormal base for tangent space samples, refering: https://graphics.pixar.com/library/OrthonormalB/paper.pdf
std::shared_ptr<glm::mat3> IntersectRecord::genTBN(){
    normal_=glm::normalize(normal_);

    glm::vec3 b1,b2;
    float sign = std::copysign(1.0f, normal_.z);  // +1 if n.z >= 0, -1 if n.z < 0
    const float a = -1.0f / (sign + normal_.z);
    const float b = normal_.x * normal_.y * a;
    
    b1.x = 1.0f + sign * normal_.x * normal_.x * a;
    b1.y = sign * b;
    b1.z = -sign * normal_.x;
    
    b2.x = b;
    b2.y = sign + normal_.y * normal_.y * a;
    b2.z = -normal_.y;

    return std::make_shared<glm::mat3>(b1,b2,normal_);
}

// transform ray from world space to tangent space local to the hit point.
glm::vec3 IntersectRecord::ray2TangentSpace(const glm::vec3& world_dir){
    if(!TBN_)
        TBN_=genTBN();

    glm::mat3 invTBN=glm::transpose(*TBN_);           // TBN is othonormal so transposion equals to inversion
    glm::vec3 local_dir=invTBN*world_dir;             // world_dir is normalized so local_dir should be normalized by nature
    assert(fabs(glm::length(local_dir)-1)<1e-6);

    return local_dir;
}

// transform the sampled Wi from tangent space to world space.
glm::vec3 IntersectRecord::wi2WorldSpace(const glm::vec3& wi){
    if(!TBN_)
        TBN_=genTBN();

    return (*TBN_)*wi;
}



/*-----------------------------------------------------------*/
/*------------------------Htriangle--------------------------*/
/*-----------------------------------------------------------*/


bool Htriangle::anyHit(const Ray& ray)const{
    glm::vec3 e1=points[1]->pos_-points[0]->pos_;
    glm::vec3 e2=points[2]->pos_-points[0]->pos_;
    glm::vec3 s=ray.origin_-points[0]->pos_;
    glm::vec3 s1=glm::cross(ray.dir_,e2);
    glm::vec3 s2=glm::cross(s,e1);

    float det=glm::dot(s1,e1);
    // parallel or degeneration of triangle
    if(fabs(det)<srender::EPSILON) 
        return false;

    float factor=1.0/det;

    float t=factor*glm::dot(s2,e2);
    // only keep the forward hit
    if(t<0.f)   
        return false;

    float b1=factor*glm::dot(s1,s);
    float b2=factor*glm::dot(s2,ray.dir_);

    if(b1<-srender::EPSILON||b2<-srender::EPSILON||1-b1-b2<-srender::EPSILON)
        return false;

    return true;
}

/**
 * @brief Moller Trumbore intersection algorithm
 * @param ray :must has been transformed into triangle's model space
 * @param inst : the record are all in model space
 * @return true : do have a intersection
 */
bool Htriangle::rayIntersect(const Ray& ray,IntersectRecord& inst)const{
    assert(fabs(glm::length(ray.dir_)-1.0)<1e-6);

    glm::vec3 e1=points[1]->pos_-points[0]->pos_;
    glm::vec3 e2=points[2]->pos_-points[0]->pos_;
    glm::vec3 s=ray.origin_-points[0]->pos_;
    glm::vec3 s1=glm::cross(ray.dir_,e2);
    glm::vec3 s2=glm::cross(s,e1);

    float det=glm::dot(s1,e1);
    // parallel or degeneration of triangle
    if(fabs(det)<srender::EPSILON) 
        return false;

    float factor=1.0/det;

    float t=factor*glm::dot(s2,e2);
    // only keep the forward hit
    if(t<0.f)   
        return false;

    float b1=factor*glm::dot(s1,s);
    float b2=factor*glm::dot(s2,ray.dir_);

    if(b1<0||b2<0||1-b1-b2<0)   // give a -eps tolerance?
        return false;

    if(ray.acceptT(t)){
        // record the nearest hit.
        if(inst.t_>t){
            inst.pos_=ray.origin_+t*ray.dir_;
            inst.t_=t;

            // interpolate Shading Normal in local space
            auto local_norm=(1-b1-b2)*points[0]->norm_+b1*points[1]->norm_+b2*points[2]->norm_; // this is always towards the front face of mesh
            if(glm::dot(local_norm,ray.dir_)>0)
                local_norm=-local_norm;     // reverse the shading norm to always keep the inverse direction of incident ray 
            inst.normal_=glm::normalize(local_norm);

            // interpolate UV
            for(int i=0;i<2;++i){
                inst.uv_[i]=(1-b1-b2)*points[0]->uv_[i]+b1*points[1]->uv_[i]+b2*points[2]->uv_[i]; 
            }

            inst.material_=material_;
            
        }
        return true;
    }
    else   return false;

}
