#pragma once
#include"common/common_include.h"
#include"texture.h"

enum class MltMember{
    Ambient,
    Diffuse,
    Specular,
};

/**
 * @brief specify the components of a object's material, which will direct the build of BSDF model later 
 * 
 */
enum class MtlType{
    NotInit     =0,

    // Scattering type
    Diffuse     =1<<1,
    Specular    =1<<2,
    Glossy      =Diffuse|Specular, 
    // TODO : use Cook-Torrance to unify glossy material 

    // Emission type
    AreaLight   =1<<10,

    NonEmissive =Diffuse|Specular|Glossy,
    Emissive = AreaLight,
};
inline MtlType operator|(const MtlType& s1,const MtlType& s2){
    return (MtlType)((int)(s1)|(int)(s2));
}
inline MtlType operator&(const MtlType& s1,const MtlType& s2){
    return (MtlType)((int)(s1)&(int)(s2));
}
inline MtlType operator^(const MtlType& s1,const MtlType& s2){
    return (MtlType)((int)(s1)^(int)(s2));
}

class Material{
public:
    // init type as non-emissive
    Material():type_(MtlType::NotInit){}

    void setProperties(glm::vec3 am,glm::vec3 di,glm::vec3 sp,glm::vec3 tr=glm::vec3(1),float ns=1,float ni=1);
    void setName(std::string name){ name_= name; }
    void setTexture(MltMember mtype,std::string path);

    // initialize scattering type
    void initScattringType();

    // initialize emission type if any and Specify which type of emitter it is and its radiance
    void initEmissionType(MtlType type,const glm::vec3& radiance );

    std::shared_ptr<Texture> getTexture(MltMember mtype)const;
    glm::vec3 getAmbient()const{ return ambient_; }
    glm::vec3 getDiffuse()const{ return diffuse_; }
    /**
     * @brief read texture and return diffuse color in [0,1]
     */
    glm::vec3 getDiffuse(float u,float v )const{ 
        if(u<0||v<0)
            return diffuse_;
        auto value=dif_texture_->getColorBilinear(u,v)/255.0f;
        return value;
    }

    glm::vec3 getSpecular()const{ return specular_; }
    std::string getName()const{ return name_; }
    float getShininess() const{ return shininess_;}

    glm::vec3 getEmit()const{return radiance_rgb_;}

    bool isEmissive(){
        return (bool)(type_&MtlType::Emissive);
    }

public:
    MtlType type_;
    std::string name_;

    glm::vec3 ambient_;     // local illumination need this to simply simulate environment light.
    glm::vec3 diffuse_;     // the diffuse reflectance of material, diffuse_path_ is the texture file path.
    glm::vec3 specular_;    // the specular reflectance of material, specular_path_ is the texture file path.
    float shininess_=32;    // the exponent of phong lobe.
    float ior_;             // the Index of Refraction(IOR) of transparent object like glass and water.
    glm::vec3 transmittance_; 

    std::string ambient_path_;
    std::string diffuse_path_;
    std::string specular_path_;

    std::shared_ptr<Texture> amb_texture_=nullptr;
    std::shared_ptr<Texture> dif_texture_=nullptr;
    std::shared_ptr<Texture> spe_texture_=nullptr;

    // only available for emissive light
    glm::vec3 radiance_rgb_;    
    

};
