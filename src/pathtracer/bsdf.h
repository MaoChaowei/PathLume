#pragma once
#include"common/common_include.h"
#include"sample.h"
#include"hitem.h"
#include"enumtypes.h"

struct BSDFRecord{
public:
    IntersectRecord& inst;
    Sampler& sampler;

    // wo and wi are both in tangent space local to the intersection point, and point out of the surface 
    glm::vec3 wo;
    glm::vec3 wi;

    // costheta is the cosine value between normal of the hit point and world-wi, or in tangent space is dot((0,0,1),wi)=wi_z. 
    // But it's always positive by "max(0.f,wi_z)" 
    float costheta;

    // pdf of sampling wi
    float pdf;

    // The actual BSDF value, which is calculated by `evalBSDF`
    glm::vec3 bsdf_val;

public:
    /* ray_dir is the direction of the incident ray, which is opposed to the normal of the surface.  */ 
    BSDFRecord(IntersectRecord& i,Sampler& s,glm::vec3 ray_dir):inst(i),sampler(s){
        wo=inst.ray2TangentSpace(ray_dir);
    }
    // if bsdf_val or pdf is too small,return false.
    bool isValid(){
        for(int i=0;i<3;++i){
            if(bsdf_val[i]<srender::EPSILON)
                return false;
        }
        if(pdf<srender::EPSILON)
            return false;
        return true;
    }
};



/**
 * @brief BSDF modelizes the sampling of scattering models.
 * It exposes functions such as: evaluating(bsdf value) and 
 * sampling([0,1]^2->[theta,phi]) the model, and querying the pdf(w_i).
 * Note that Wi and Wo are both in tangent space local to the intersection point.
 */
class BSDF{
public:
    BSDF(BSDFType type):bsdf_type_(type){}
    virtual ~BSDF(){}

    // wo and wi should be in tangent space
    virtual glm::vec3 evalBSDF(const glm::vec3& wo,const glm::vec3& wi)const {
        throw std::runtime_error("Confront Base class's virtural function...");
        return glm::vec3(0);
    }
    // wo and wi should be in tangent space
    virtual float calculatePDF(const glm::vec3& wo,const glm::vec3& wi)const{
        throw std::runtime_error("Confront Base class's virtural function...");
        return 0;
    }
    // wo and wi should be in tangent space
    virtual void sampleBSDF(BSDFRecord& bsdfRec)const=0;

    // each BSDF has a weight, for example, LambertianBSDF's weight is albedo(Kd)
    virtual float getWeight()const=0;

    /**
     * @brief static function for sampling in hemishpere with cos weighted
     * 
     * @param uniform : samples in [0,1]^2
     * @return float : pdf(wi)
     */
    static float SpHSphereCosWeight(float& theta,float& phi,const glm::vec2& uniform){
        theta=acos(sqrt(uniform[0]));
        phi=2*srender::PI*uniform[1];
        
        return cos(theta)*srender::INV_PI;
    }

protected:
    BSDFType bsdf_type_;
};

/**
 * @brief `BSDFlist` is a abstract for multiple bsdfs, which will sample a wi among all the bsdfs by their weights
 * 
 */
class BSDFlist:public BSDF{
public:
    BSDFlist(BSDFType type):BSDF(type),total_weight_(0.f){}
    // Each material
    void insertBSDF(std::shared_ptr<const BSDF> bsdf){
        bsdfs_.emplace_back(bsdf);
        float w=bsdf->getWeight();
        total_weight_+=w;
        weights_.emplace_back(w);
    }

    void caluculateWeights(){
        for(int i=0;i<weights_.size();++i){
            weights_[i]/=total_weight_;
        }
    }

    // wo and wi should be in tangent space
    glm::vec3 evalBSDF(const glm::vec3& wo,const glm::vec3& wi)const override{
        return glm::vec3(0);
    }

    // wo and wi should be in tangent space
    float calculatePDF(const glm::vec3& wo,const glm::vec3& wi)const override{
        return 0;
    }
    // wo and wi should be in tangent space
    void sampleBSDF(BSDFRecord& bsdfRec)const override{

    }

    float getWeight()const{
        return total_weight_;
    }

private:

    std::vector<float> weights_;
    std::vector<std::shared_ptr<const BSDF>> bsdfs_;
    float total_weight_;

};

class LambertRefBSDF:public BSDF{
public:
    LambertRefBSDF(glm::vec3 r,BSDFType type=BSDFType::LambertReflection):BSDF(type),albedo_(r){}

    glm::vec3 evalBSDF(const glm::vec3& wo,const glm::vec3& wi)const override{
        return albedo_*srender::INV_PI;
    }
    float calculatePDF(const glm::vec3& wo,const glm::vec3& wi)const override{
        return std::max(0.f,wi.z)*srender::INV_PI;
    }

    void sampleBSDF(BSDFRecord& bsdfRec)const override{

        // sample the hemisphere,get wi
        glm::vec2 uniform=bsdfRec.sampler.getSample2D();
        float theta,phi;
        bsdfRec.pdf=this->SpHSphereCosWeight(theta,phi,uniform);
        
        float x=1.0*sin(theta)*cos(phi);
        float y=1.0*sin(theta)*sin(phi);
        float z=1.0*cos(theta);     // z is the direction of normal in this local space
        bsdfRec.wi=glm::vec3(x,y,z);        
        bsdfRec.bsdf_val=evalBSDF(bsdfRec.wo,bsdfRec.wi);
        bsdfRec.costheta=std::max(bsdfRec.wi.z,0.f);

    }

    float getWeight()const{
        return albedo_[0]+albedo_[1]+albedo_[2];
    }

private:
    glm::vec3 albedo_;

};

