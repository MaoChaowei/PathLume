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
    static void SpHSphereCosWeight(float& theta,float& phi,const glm::vec2& uniform){
        theta=acos(sqrt(uniform[0]));
        phi=2*srender::PI*uniform[1];
    }

    static glm::vec3 polar2Cartesian(float theta,float phi){
        float x=1.0*sin(theta)*cos(phi);
        float y=1.0*sin(theta)*sin(phi);
        float z=1.0*cos(theta);     
        return glm::vec3(x,y,z);   
    }

public:
    BSDFType bsdf_type_;
    
};

/**
 * @brief `BSDFlist` is an abstract for multiple bsdfs, which will sample a wi among all the bsdfs by their weights
 * 
 */
class BSDFlist:public BSDF{
public:
    BSDFlist(BSDFType type=BSDFType::BSDFLIST):BSDF(type),total_weight_(0.f){}

    void insertBSDF(std::shared_ptr<const BSDF> bsdf){
        bsdfs_.emplace_back(bsdf);
        float w=bsdf->getWeight();

        total_weight_+=w;
        weights_.emplace_back(w);
    }

    void initWeights(){
        if(!bsdfs_.size()||!total_weight_)  return;

        float temp=0;
        for(int i=0;i<bsdfs_.size();++i){
            weights_[i]/=total_weight_;
            temp+= weights_[i];
            cdf_.emplace_back(temp);
        }
        assert(fabs(cdf_.back() - 1.0f) < srender::EPSILON);
    }

    /**
     * @brief only evaluta spceular bsdf for MIS
     * 
     * @param rec 
     */
    void evalBSDF(BSDFRecord& rec)const override{
        // check wi is prepared
        assert(fabs(glm::length(rec.wi)-1.0)<srender::EPSILON);
        rec.pdf=0.f;
        rec.bsdf_val=glm::vec3(0.f);

        for(int i=0;i<bsdfs_.size();++i){
            BSDFRecord temp(rec.inst,rec.sampler,rec.wo,rec.wi);
            bsdfs_[i]->evalBSDF(temp);
            rec.pdf+=weights_[i]*temp.pdf;
            rec.bsdf_val+=weights_[i]*temp.bsdf_val;
        }


    }

    void sampleBSDF(BSDFRecord& bsdfRec)const override{
        // choose a bsdf randomly
        float rd=bsdfRec.sampler.pcgRNG_.nextFloat();
        int chosen=binarySearchBRDF(rd);

        // sample this bsdf
        bsdfs_[chosen]->sampleBSDF(bsdfRec);
        bsdfRec.bsdf_type=bsdfs_[chosen]->bsdf_type_;

    }

    float getWeight()const override{
        return total_weight_;
    }
 
private:
    // find the i such that cdf_[i]<u<=cdf_[i+1]
    int binarySearchBRDF(float u)const{
        int lt=0;
        int rt=cdf_.size()-1;
        while(lt<rt){
            int mid=(lt+rt)/2;
            if(cdf_[mid]<u) lt=mid+1;
            else rt=mid;
        }
        return lt;
    }

    std::vector<float> cdf_;    // the cdf of weights_
    std::vector<float> weights_;
    std::vector<std::shared_ptr<const BSDF>> bsdfs_;
    float total_weight_;

};

class LambertBRDF:public BSDF{
public:
    LambertBRDF(glm::vec3 r,BSDFType type=BSDFType::LambertReflection):BSDF(type),albedo_(r){}

    void evalBSDF(BSDFRecord& rec)const override{
        rec.bsdf_val=albedo_*srender::INV_PI;
        rec.pdf=std::max(0.f,rec.wi.z)*srender::INV_PI;
    }

    void sampleBSDF(BSDFRecord& bsdfRec)const override{

        // sample the hemisphere,get wi
        glm::vec2 uniform=bsdfRec.sampler.getSample2D();
        float theta,phi;
        this->SpHSphereCosWeight(theta,phi,uniform);   
        bsdfRec.wi=polar2Cartesian(theta,phi);
        bsdfRec.costheta=std::max(bsdfRec.wi.z,0.f);

        bsdfRec.bsdf_type=bsdf_type_;

    }

    float getWeight()const override{
        return albedo_[0]+albedo_[1]+albedo_[2];
    }

private:
    glm::vec3 albedo_;

};

class SpecularBRDF:public BSDF{
public:
    SpecularBRDF(glm::vec3 ks,BSDFType type=BSDFType::PerfectReflection):BSDF(type),albedo_(ks){}

    void evalBSDF( BSDFRecord& rec)const override{
        auto reflected=perfectReflect(glm::vec3(0,0,1),rec.wo);
        if(glm::length(reflected-rec.wi)<srender::EPSILON){
            rec.bsdf_val = albedo_;
            rec.pdf=1.0;
        }
        else{
            rec.bsdf_val = glm::vec3(0.f);
            rec.pdf=0.f;
        }
    }


    void sampleBSDF(BSDFRecord& bsdfRec)const override{
        
        bsdfRec.wi=perfectReflect(glm::vec3(0,0,1),bsdfRec.wo);
        bsdfRec.bsdf_val =albedo_;
        bsdfRec.pdf=1.0;
        bsdfRec.costheta=std::max(bsdfRec.wi.z,0.f);

        bsdfRec.bsdf_type=bsdf_type_;
    }

    float getWeight()const override{
        return albedo_[0]+albedo_[1]+albedo_[2];
    }

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
    BPhongSpecularBRDF(glm::vec3 ks,float ns,BSDFType type=BSDFType::BlinnPhongSpecular):BSDF(type),Ks(ks),Ns(ns){}

    void evalBSDF( BSDFRecord& rec)const override{
        if(rec.wi.z<0.f){
            rec.pdf=0;
            rec.bsdf_val=glm::vec3(0);
            return;
        }
        glm::vec3 h=glm::normalize(rec.wi+rec.wo);

        float cos_Ns=pow(std::max(h.z,0.f),Ns);
        float temp=(Ns+2)*(0.5*srender::INV_PI)*cos_Ns;
        float coef=0.25/(glm::dot(rec.wo,h));
        
        rec.bsdf_val=Ks*temp*coef;

        rec.pdf=temp*h.z*coef;
    }

    void sampleBSDF(BSDFRecord& bsdfRec)const override{
        // sample the pdf,get wi
        glm::vec2 uniform=bsdfRec.sampler.getSample2D();
        float costheta=pow(uniform[0],1.0/(Ns+2));
        float theta=acos(costheta);
        float phi=2*srender::PI*uniform[1];
        glm::vec3 h=polar2Cartesian(theta,phi);

        bsdfRec.wi=getReflect(h,bsdfRec.wo);
        if(bsdfRec.wi.z<0.f){
            bsdfRec.pdf=0.f;
            bsdfRec.bsdf_val=glm::vec3(0);
            return;
        }
        bsdfRec.costheta=bsdfRec.wi.z;

        bsdfRec.bsdf_type=bsdf_type_;
        
    }

    float getWeight()const override{
        return 2.0*(Ks[0]+Ks[1]+Ks[2]);
    }

private:
    glm::vec3 getReflect(const glm::vec3& h,const glm::vec3& wo)const{
        auto wi=2*glm::dot(wo,h)*h-wo;
        assert(fabs(glm::length(wi)-1)<srender::EPSILON);   // wi should be normalized by nature
        return wi;
    }


    glm::vec3 Ks;
    float Ns;
};