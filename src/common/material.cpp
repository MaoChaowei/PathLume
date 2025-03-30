#include"material.h"

void Material::setProperties(glm::vec3 am,glm::vec3 di,glm::vec3 sp,glm::vec3 tr,float ns,float ni){
    ambient_=am;
    diffuse_=di;
    specular_=sp;
    shininess_=ns;
    ior_=ni;
    transmittance_=tr;
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

// initialize scattering type
void Material::initScattringType(){
    if((type_==MtlType::NotInit)){
        if(dif_texture_||diffuse_[0]||diffuse_[1]||diffuse_[2])
            type_=type_|MtlType::Diffuse;
        if(spe_texture_||specular_[0]||specular_[1]||specular_[2])
            type_=type_|MtlType::Specular;
        
    }
}

// initialize emission type if any and Specify which type of emitter it is and its radiance
void Material::initEmissionType(MtlType type,const glm::vec3& radiance ){
    if((int)(type&MtlType::Emissive)){
        type_=type_|type;
        radiance_rgb_=radiance;
    }
}
