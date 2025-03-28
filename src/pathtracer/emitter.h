#pragma once
#include"common_include.h"
#include"vertex.h"
#include"ray.h"
#include"utils.h"
#include"hitem.h"

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
    /**
     * @brief add a triangle to emitter
     * @param a~c the pointer to a emissive triangle
     * @param radiance the rgb radiance of the triangle
     */
    void addEmitter(const Vertex * a,const Vertex * b,const Vertex * c,glm::vec3 n,glm::vec3 radiance);

    /**
     * @brief clean the emitter when scene has changed
     */
    void clear();

    /**
     * @brief Get the Total Radiance Weight of the scene, which is the sum of radiance*area of the whole scene
     */
    float getWeight(){return totalWeight_;}

    /**
     * @brief sample a light from the intersection point
     * 
     * @param src_pos world position of the intersection point
     * @param lsRec record some sampled results
     * @param u0 sample a triangle
     * @param u1&u2 sample a position in the sampled triangle
     */
    void sampleLight(const glm::vec3& src_pos,LightSampleRecord& lsRec,float u0,float u1,float u2)const;

    /**
     * @brief Given an incident wi (`ray`) and its intersection `inst` with emitter, return its pdf(wi)
     */
    float getSamplePDF(const Ray& ray,const IntersectRecord& inst)const;

    /**
     * @brief Set the Presum of the emitters' weights 
     * 
     */
    void setPreSum();

private:

    /**
     * @brief Get a Emit Face Index
     * 
     * @param u : sample in U~[0,1]
     * @return uint32_t : the index of the sampled emissive triangle
     */
    uint32_t binarySearchEmitFace(float u)const;

private:

    
    std::vector<float> presum_;// prefix sum of radiance_rgb*area. the i-th number in presum_ means the sum of weights in etris_[0~i-1]
    std::vector<EmitTriangle> etris_;
    float totalWeight_;


};