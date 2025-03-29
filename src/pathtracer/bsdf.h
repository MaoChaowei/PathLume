#pragma once
#include"common/common_include.h"
#include"sample.h"
#include"hitem.h"
#include"enumtypes.h"

/**
 * @brief Encapsulate variables for Sampling and Evaluate BSDF and its PDF as well.
 * 
 */
struct BSDFRecord{
public:
    IntersectRecord& inst;
    Sampler& sampler;

    // wo and wi are both in tangent space local to the intersection point, and point out of the surface 
    glm::vec3 wo;
    glm::vec3 wi;

    // costheta is the cosine value between normal of the hit point and world-wi, which in tangent space is dot((0,0,1),wi)=wi_z. 
    // it's always positive by "max(0.f,wi_z)" 
    float costheta;

    // pdf of sampling wi
    float pdf;

    // The actual BSDF value, which is calculated by `evalBSDF`
    glm::vec3 bsdf_val;

    BSDFType bsdf_type;

public:

    // Init BSDFRecord without knowing wi. --> To sample and evaluate
    // ray_dir is in the same direction as wo 
    BSDFRecord(IntersectRecord& i,Sampler& s,glm::vec3 ray_dir):inst(i),sampler(s){
        wo=inst.ray2TangentSpace(ray_dir);
    }

    // Init BSDFRecord already knowing wi. --> To evaluate only
    // ray_dir is in the same direction as wo 
    BSDFRecord(IntersectRecord& i,Sampler& s,glm::vec3 wo,glm::vec3 wi):inst(i),sampler(s),wo(wo),wi(wi){}

    // if bsdf_val or pdf is too small,return false.
    // so if I don't want a sample, simply set pdf to -1
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
 * @brief BSDF modelizes the sampling and evaluation of scattering models.
 * It exposes functions such as: evaluating(bsdf value) and 
 * sampling([0,1]^2->[theta,phi]) the model, and querying the pdf(w_i).
 * Note that Wi and Wo are both in tangent space local to the hit point 
 * and both point in the direciton of Normal
 */
class BSDF{
public:
    BSDF(BSDFType type):bsdf_type_(type){}
    virtual ~BSDF(){}

    /**
     * @brief Sample a wi in tangent space with the information stored in BSDFRecord,including wo,inst,sampler and such.
     *        Have to fill wi and costheta
     */
    virtual void sampleBSDF(BSDFRecord& bsdfRec)const=0;

    /**
     * @brief After the sampling of wi, this function takes charge of calculate bsdf_value and pdf.
     *        So remember to sample before calling this.
     */
    virtual void evalBSDF(BSDFRecord& rec)const=0;

    /**
     * @brief When choosing among different bsdfs, each BSDF are required to offer a weight, 
     *        for example, LambertianBSDF's weight is albedo(Kd)
     */
    virtual float getWeight()const=0;

    /**
     * @brief static function for sampling in hemishpere with cos weighted
     * 
     * @param uniform : samples in [0,1]^2
     */
    static void SpHSphereCosWeight(float& theta,float& phi,const glm::vec2& uniform);

    static glm::vec3 polar2Cartesian(float theta,float phi);

public:
    BSDFType bsdf_type_;
    float prob_;
    
};

/**
 * @brief `BSDFlist` is an abstract for multiple bsdfs, which will sample a wi among all the bsdfs by their weights
 * 
 */
class BSDFlist:public BSDF{
public:
    BSDFlist(BSDFType type=BSDFType::EMPTY):BSDF(type),total_weight_(0.f){}

    void insertBSDF(std::shared_ptr<const BSDF> bsdf);

    void initWeights();

    /**
     * @brief For each hit, randomly choose a bsdf ,and record the chosen type in member: bsdf_type_
     * 
     * @param rd : a random number in [0,1)
     */
    void randomSelectBSDF(float rd){
        // choose a bsdf randomly
        chosenIdx_=binarySearchBRDF(rd);
        this->bsdf_type_=bsdfs_[chosenIdx_]->bsdf_type_;
    }

    // must be called after `randomSelectBSDF`
    void evalBSDF(BSDFRecord& rec)const override;

    // must be called after `randomSelectBSDF`
    void sampleBSDF(BSDFRecord& bsdfRec)const override;

    float getWeight()const override;
 
private:

    /**
     * @brief find the i such that cdf_[i]<u<=cdf_[i+1]
     * 
     * @param u : random number in [0,1)
     * @return int : the chosen index of bsdf in bsdfs_
     */
    int binarySearchBRDF(float u)const;

    std::vector<float> cdf_;    // the cdf of probablity; cdf_[i+1] is the sum of bsdfs_[0~i].prob_
    std::vector<float> weights_;
    std::vector<std::shared_ptr<const BSDF>> bsdfs_;
    float total_weight_;
    int chosenIdx_;

};

class LambertBRDF:public BSDF{
public:
    LambertBRDF(glm::vec3 r,BSDFType type=BSDFType::LambertReflection):BSDF(type),albedo_(r){
        prob_=0.5;
    }

    void evalBSDF(BSDFRecord& rec)const override;

    void sampleBSDF(BSDFRecord& bsdfRec)const override;

    float getWeight()const override;

private:
    glm::vec3 albedo_;

};

class SpecularBRDF:public BSDF{
public:
    SpecularBRDF(glm::vec3 ks,BSDFType type=BSDFType::PerfectReflection):BSDF(type),albedo_(ks){
        prob_=0.7;
    }

    void evalBSDF( BSDFRecord& rec)const override;

    void sampleBSDF(BSDFRecord& bsdfRec)const override;

    float getWeight()const override;

private:
    glm::vec3 perfectReflect(const glm::vec3& normal,const glm::vec3& wo)const{
        auto wi=2*glm::dot(wo,normal)*normal-wo;
        assert(fabs(glm::length(wi)-1)<srender::EPSILON);   // wi should be normalized by nature
        return wi;
    }

    glm::vec3 albedo_;

};


class BPhongSpecularBRDF:public BSDF{
public:
    BPhongSpecularBRDF(glm::vec3 ks,float ns,BSDFType type=BSDFType::BlinnPhongSpecular):BSDF(type),Ks(ks),Ns(ns){
        prob_=0.5;
    }

    void evalBSDF( BSDFRecord& rec)const override;

    void sampleBSDF(BSDFRecord& bsdfRec)const override;

    float getWeight()const override;

private:
    glm::vec3 getReflect(const glm::vec3& h,const glm::vec3& wo)const{
        auto wi=2*glm::dot(wo,h)*h-wo;
        assert(fabs(glm::length(wi)-1)<srender::EPSILON);   // wi should be normalized by nature
        return wi;
    }

    // glm::vec3 Fresnel(const glm::vec3& F0,float NdotV)const{
    //     float cos=std::max(NdotV,0.f);
    //     float cos5=pow(1-cos,5);
    //     return F0+(glm::vec3(1)-F0)*glm::vec3(cos5);
    // }


    glm::vec3 Ks;
    float Ns;
};