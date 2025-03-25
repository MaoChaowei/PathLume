#pragma once
#include"common_include.h"
#include"vertex.h"
#include"ray.h"

/**
 * @brief A bridge struct connects hittable object and corresponding hitting method
 * 
 */
struct EmitTriangle{
    EmitTriangle(const Vertex * a,const Vertex * b,const Vertex * c,glm::vec3 n,glm::vec3 radiance)
    :v0(a),v1(b),v2(c),normal(n),radiance_rgb(radiance){
        calculateArea();
    }

    const Vertex * v0;
    const Vertex * v1;
    const Vertex * v2;
    glm::vec3 normal;       // point to the front face
    glm::vec3 radiance_rgb; // radiance value for each direction
    float area;

    void calculateArea(){
        area=0.5*glm::length(glm::cross(v1->w_pos_-v0->w_pos_,v2->w_pos_-v0->w_pos_));
    }

    float getWeight(){
        return area*utils::getLuminance(radiance_rgb);
    }
};

/**
 * @brief Encapsulate necessary info for sampling a light
 *  - shadow_ray_: Null means sample failed
 *  - dist_ : actual distance between src and dst
 *  - value_ : for Mento Carlo: Radiance*cos(theta')/(dist^2*A)
 *  - pdf_ : pdf in dWi measurement rather than dA, that is G/area (for throwing outlier samples
 */
struct LightSampleRecord{
    std::shared_ptr<Ray> shadow_ray_=nullptr;  
    float dist_=0;                      // distance between src_pos and sample_pos
    glm::vec3 value_=glm::vec3(0.f);    // for Mento Carlo: Radiance*cos(theta')/(dist^2*A)
    float pdf_=0;                       // pdf in dWi measurement rather than dA, that is G/area
};

/**
 * @brief takes charge of all the emittive triangles and provides corresponding sampling methods
 * 
 */
class Emitters{
public:
    
    void addEmitter(const Vertex * a,const Vertex * b,const Vertex * c,glm::vec3 n,glm::vec3 radiance){
        etris_.emplace_back(a,b,c,n,radiance);
    }
    void clear(){
        presum_.clear();
        etris_.clear();
        totalWeight_=0;
    }
    float getWeight(){return totalWeight_;}

    /**
     * @brief sample a light from the intersection point
     * 
     * @param src_pos world position of the intersection point
     * @param lsRec record some sampled results
     * @param u0 sample a triangle
     * @param u1&u2 sample a position in the sampled triangle
     */
    void sampleLight(const glm::vec3& src_pos,LightSampleRecord& lsRec,float u0,float u1,float u2)const{
        if(!etris_.size()){
            return;
        }
        // sample a triangle
        uint32_t tridx=getEmitFace(u0);
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

    /**
     * @brief Given an incident wi (`ray`) and its intersection `inst` with emitter, return its pdf(wi)
     */
    float getSamplePDF(const Ray& ray,const IntersectRecord& inst)const{

        if(glm::dot(ray.dir_,inst.normal_)>0.f)
            throw std::runtime_error("Emitter:getSamplePDF->glm::dot(ray.dir_,inst.normal_)>0.f");

        float costheta=std::max(glm::dot(inst.normal_,glm::normalize(-ray.dir_)),0.f);    // ignore the back face

        if(inst.t_<srender::EPSILON)
            return 0.f;

        float squred_dist=inst.t_*inst.t_;

        float G=costheta/squred_dist;

        return 1.0*(utils::getLuminance(inst.material_->radiance_rgb_))/(totalWeight_*G); 
        

    }

    void setPreSum(){
        int num=etris_.size();
        presum_.resize(num+1);

        presum_[0]=0;
        for(int i=0;i<num;++i){
            presum_[i+1]=presum_[i]+etris_[i].getWeight();
        }

        totalWeight_=presum_[num];
    }

private:

    /**
     * @brief Get a Emit Face Index
     * 
     * @param u : sample in U~[0,1]
     * @return uint32_t : the index of the sampled emissive triangle
     */
    uint32_t getEmitFace(float u)const{
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

private:

    // prefix sum of radiance_rgb*area. the i-th number in presum_ means the sum of weights in etris_[0~i-1]
    std::vector<float> presum_;
    std::vector<EmitTriangle> etris_;
    float totalWeight_;


};