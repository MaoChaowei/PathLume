#include"material.h"

void Material::setADS(glm::vec3 am,glm::vec3 di,glm::vec3 sp,float shininess){
    ambient_=am;
    diffuse_=di;
    specular_=sp;
    shininess_=shininess;
}

void Material::setTexture(MltMember mtype,std::string path){
    switch(mtype){
        case MltMember::Ambient:
            ambient_path_=path;
            amb_texture_=std::make_shared<Texture>(path);
            break;
        case MltMember::Diffuse:
            diffuse_path_=path;
            dif_texture_=std::make_shared<Texture>(path);
            break;
        case MltMember::Specular:
            specular_path_=path;
            spe_texture_=std::make_shared<Texture>(path);
            break;            
    }
}

std::shared_ptr<Texture> Material::getTexture(MltMember mtype)const{
    switch(mtype){
        case MltMember::Ambient:
            return amb_texture_;
        case MltMember::Diffuse:
            return dif_texture_;
        case MltMember::Specular:
            return spe_texture_;       
    }
    std::cout<<"std::shared_ptr<Texture> getTexture(MltMember mtype): unknow input!\n";
    return nullptr;
}