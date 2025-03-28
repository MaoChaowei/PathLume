#include"emitter.h"

/*-----------------------------------------------------------*/
/*------------------------Emitters---------------------------*/
/*-----------------------------------------------------------*/

void Emitters::addEmitter(const Vertex * a,const Vertex * b,const Vertex * c,glm::vec3 n,glm::vec3 radiance){
    etris_.emplace_back(a,b,c,n,radiance);
}
void Emitters::clear(){
    presum_.clear();
    etris_.clear();
    totalWeight_=0;
}


void Emitters::sampleLight(const glm::vec3& src_pos,LightSampleRecord& lsRec,float u0,float u1,float u2)const{
    if(!etris_.size()){
        return;
    }
    // sample a triangle
    uint32_t tridx=binarySearchEmitFace(u0);
    const EmitTriangle& tri=etris_[tridx];

    // sample a point
    if(u1+u2>1){
        u1=1-u1;
        u2=1-u2;
    }

    // get Shading normal and shading point
    glm::vec3 dst_pos=(1-u1-u2)*tri.v0->w_pos_+u1*tri.v1->w_pos_+u2*tri.v2->w_pos_;
    glm::vec3 light_norm=glm::normalize((1-u1-u2)*tri.v0->w_norm_+u1*tri.v1->w_norm_+u2*tri.v2->w_norm_);

    // calculate sample value
    glm::vec3 dist_vec=dst_pos-src_pos;
    lsRec.shadow_ray_=std::make_shared<Ray>(src_pos,dist_vec);

    float squred_dist=glm::dot(dist_vec,dist_vec);
    lsRec.dist_=glm::sqrt(squred_dist);

    float costheta=std::max(glm::dot(light_norm,glm::normalize(-dist_vec)),0.f);    // ignore the back face

    float G=costheta/squred_dist;
    
    float inv_pdf_w=(totalWeight_*G)/(utils::getLuminance(tri.radiance_rgb));  
    lsRec.pdf_=1.0/inv_pdf_w;

    glm::vec3 Le= tri.radiance_rgb;

    lsRec.value_ = Le*inv_pdf_w;
}


float Emitters::getSamplePDF(const Ray& ray,const IntersectRecord& inst)const{

    if(glm::dot(ray.dir_,inst.normal_)>0.f)
        throw std::runtime_error("Emitter:getSamplePDF->glm::dot(ray.dir_,inst.normal_)>0.f");

    float costheta=std::max(glm::dot(inst.normal_,glm::normalize(-ray.dir_)),0.f);    // ignore the back face

    if(inst.t_<srender::EPSILON)
        return 0.f;

    float squred_dist=inst.t_*inst.t_;

    float G=costheta/squred_dist;

    return 1.0*(utils::getLuminance(inst.material_->radiance_rgb_))/(totalWeight_*G); 
    

}

void Emitters::setPreSum(){
    int num=etris_.size();
    presum_.resize(num+1);

    presum_[0]=0;
    for(int i=0;i<num;++i){
        presum_[i+1]=presum_[i]+etris_[i].getWeight();
    }

    totalWeight_=presum_[num];
}


uint32_t Emitters::binarySearchEmitFace(float u)const{
    u=std::clamp(u,0.0f,srender::OneMinusEpsilon);

    // find the first emissive triangle(idx=i-1) in etris_ such that presum_[i]>=totalweight*u
    float threshold=totalWeight_*u;
    int lt=0,rt=etris_.size()-1;

    while(lt<rt){
        int mid=(lt+rt)/2;
        if(presum_[mid+1]<threshold){
            lt=mid+1;
        }
        else{
            rt=mid;
        }
    }

    return lt;
}