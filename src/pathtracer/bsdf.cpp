#include"bsdf.h"

/*--------------------------------------------------------------------*/
/*---------------------------   BSDF  -------------------------------*/
/*--------------------------------------------------------------------*/
void BSDF::SpHSphereCosWeight(float& theta,float& phi,const glm::vec2& uniform){
    theta=acos(sqrt(uniform[0]));
    phi=2*srender::PI*uniform[1];
}

glm::vec3 BSDF::polar2Cartesian(float theta,float phi){
    float x=1.0*sin(theta)*cos(phi);
    float y=1.0*sin(theta)*sin(phi);
    float z=1.0*cos(theta);     
    return glm::vec3(x,y,z);   
}


/*--------------------------------------------------------------------*/
/*----------------------------BSDFlist -------------------------------*/
/*--------------------------------------------------------------------*/
void BSDFlist::insertBSDF(std::shared_ptr<const BSDF> bsdf){
    bsdfs_.emplace_back(bsdf);
    float w=bsdf->getWeight();
    weights_.emplace_back(w);
    total_weight_+=w;
    
}

// void BSDFlist::initWeights(){
//     if(!bsdfs_.size()||!total_weight_)  return;

//     float temp=0;
//     for(int i=0;i<bsdfs_.size();++i){
//         weights_[i]/=total_weight_;
//         temp+= weights_[i];
//         cdf_.emplace_back(temp);
//     }
//     assert(fabs(cdf_.back() - 1.0f) < srender::EPSILON);
// }

void BSDFlist::initWeights(){
    if(!bsdfs_.size()||!total_weight_)  return;

    int bsdfnum=bsdfs_.size();
    for(int i=0;i<bsdfnum;++i){
        weights_[i]/=total_weight_;
    }
    cdf_.resize(bsdfnum+1);
    cdf_[0]=0;
    for(int i=0;i<bsdfnum;++i){
        cdf_[i+1]=cdf_[i]+bsdfs_[i]->prob_;
    }
    float total=cdf_[bsdfnum];
    for(int i=0;i<bsdfnum;++i){
        cdf_[i+1]/=total;
    }
    assert(fabs(cdf_.back() - 1.0f) < srender::EPSILON);
}


void BSDFlist::evalBSDF(BSDFRecord& rec)const{

    assert(fabs(glm::length(rec.wi)-1.0)<srender::EPSILON); // check wi is prepared
    assert(chosenIdx_>=0);
    
    bsdfs_[chosenIdx_]->evalBSDF(rec);
    
    if(bsdfs_.size()>1)
        rec.bsdf_val*=(weights_[chosenIdx_]/(cdf_[chosenIdx_+1]-cdf_[chosenIdx_]));

    rec.bsdf_type=bsdfs_[chosenIdx_]->bsdf_type_;

}

void BSDFlist::sampleBSDF(BSDFRecord& bsdfRec)const{

    assert(chosenIdx_>=0);

    bsdfs_[chosenIdx_]->sampleBSDF(bsdfRec);
    bsdfRec.bsdf_type=bsdfs_[chosenIdx_]->bsdf_type_;
    
}

float BSDFlist::getWeight()const{
    return total_weight_;
}

int BSDFlist::binarySearchBRDF(float u)const{
    int lt=0;
    int rt=bsdfs_.size()-1;
    while(lt<rt){
        int mid=(lt+rt)/2;
        if(cdf_[mid+1]<u) lt=mid+1;
        else rt=mid;
    }
    return lt;
}

/*--------------------------------------------------------------------*/
/*------------------------ LambertBRDF -------------------------------*/
/*--------------------------------------------------------------------*/

void LambertBRDF::evalBSDF(BSDFRecord& rec)const{
    rec.bsdf_val=albedo_*srender::INV_PI;
    rec.pdf=std::max(0.f,rec.wi.z)*srender::INV_PI;
}

void LambertBRDF::sampleBSDF(BSDFRecord& bsdfRec)const{

    // sample the hemisphere,get wi
    glm::vec2 uniform=bsdfRec.sampler.getSample2D();
    float theta,phi;
    this->SpHSphereCosWeight(theta,phi,uniform);   
    bsdfRec.wi=polar2Cartesian(theta,phi);
    bsdfRec.costheta=std::max(bsdfRec.wi.z,0.f);

    bsdfRec.bsdf_type=bsdf_type_;

}

float LambertBRDF::getWeight()const{
    return albedo_[0]+albedo_[1]+albedo_[2];
}


/*--------------------------------------------------------------------*/
/*------------------------ SpecularBRDF -------------------------------*/
/*--------------------------------------------------------------------*/

void SpecularBRDF::evalBSDF( BSDFRecord& rec)const {
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


void SpecularBRDF::sampleBSDF(BSDFRecord& bsdfRec)const {
    
    bsdfRec.wi=perfectReflect(glm::vec3(0,0,1),bsdfRec.wo);
    bsdfRec.bsdf_val =albedo_;
    bsdfRec.pdf=1.0;
    bsdfRec.costheta=std::max(bsdfRec.wi.z,0.f);

    bsdfRec.bsdf_type=bsdf_type_;
}

float SpecularBRDF::getWeight()const {
    return albedo_[0]+albedo_[1]+albedo_[2];
}


/*--------------------------------------------------------------------*/
/*------------------- BPhongSpecularBRDF -----------------------------*/
/*--------------------------------------------------------------------*/

void BPhongSpecularBRDF::evalBSDF( BSDFRecord& rec)const{
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

void BPhongSpecularBRDF::sampleBSDF(BSDFRecord& bsdfRec)const{
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

float BPhongSpecularBRDF::getWeight()const{
    return (Ks[0]+Ks[1]+Ks[2]);
}