#pragma once
#include"common/common_include.h"
#include"texture.h"

enum class MltMember{
    Ambient,
    Diffuse,
    Specular,
};

class Material{
public:
    void setADS(glm::vec3 am,glm::vec3 di,glm::vec3 sp,float shininess=1);
    void setName(std::string name){ name_= name; }
    void setTexture(MltMember mtype,std::string path);

    std::shared_ptr<Texture> getTexture(MltMember mtype)const;



    const glm::vec3 getAmbient()const{ return ambient_; }
    const glm::vec3 getDiffuse()const{ return diffuse_; }
    const glm::vec3 getSpecular()const{ return specular_; }
    const std::string getName()const{ return name_; }
    const float getShininess() const{ return shininess_;}

public:
    std::string name_;

    glm::vec3 ambient_;
    glm::vec3 diffuse_;
    glm::vec3 specular_;
    float shininess_=32;

    std::string ambient_path_;
    std::string diffuse_path_;
    std::string specular_path_;

    std::shared_ptr<Texture> amb_texture_=nullptr;
    std::shared_ptr<Texture> dif_texture_=nullptr;
    std::shared_ptr<Texture> spe_texture_=nullptr;
    
};