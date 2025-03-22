#include"hitem.h"
#include"bsdf.h"

IntersectRecord& IntersectRecord::operator=(const IntersectRecord& inst){
    if(this==&inst)
        return *this;

    pos_=inst.pos_;
    t_=inst.t_;
    normal_=inst.normal_;
    material_=inst.material_;
    as_node_=inst.as_node_;

    return *this;
}

// use material to initialize bsdf
std::shared_ptr<BSDF> IntersectRecord::getBSDF(){
    std::shared_ptr<BSDF> bsdf_;

    // init TBN Matrix
    if(!TBN_)  TBN_=genTBN();
    assert(material_);

    auto type=this->material_->type_;
    // set emission bits to 0
    type=MtlType((int)type&(~(int)MtlType::Emissive));

    // test scattering bits
    if(type==MtlType::Diffuse){
        bsdf_=std::make_shared<LambertBRDF>(material_->diffuse_);
    }
    else if(type==MtlType::Specular){
        bsdf_=std::make_shared<SpecularBRDF>(material_->specular_);
    }
    else if(type==MtlType::Glossy){
        auto list=std::make_shared<BSDFlist>();
        // list->insertBSDF(std::make_shared<LambertBRDF>(material_->diffuse_));
        // list->insertBSDF(std::make_shared<SpecularBRDF>(material_->specular_));
        list->insertBSDF(std::make_shared<BPhongSpecularBRDF>(material_->specular_,material_->shininess_));
        list->initWeights();

        bsdf_=list;
    }
    else{
        bsdf_=std::make_shared<LambertBRDF>(material_->diffuse_);
        // std::cout<<"IntersectRecord::getBSDF():Unknown MtlType "<<material_->getName()<<", Set to Diffuse\n";
    }
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