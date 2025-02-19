#pragma once
#include"common/common_include.h"

enum class LightType{
    Dirction,
    Point,
};

class Light{
public:
    Light(){}
    Light(glm::vec3 am,glm::vec3 dif,glm::vec3 spe):ambient_(am),diffuse_(dif),specular_(spe){}

    virtual void getPointLight(glm::vec3& pos,float& c,float& li,float& quad){}
    virtual void getDirLight(glm::vec3& dir){}
    virtual ~Light() = default; 

public:
    LightType type_;

    // intensity of light
    glm::vec3 ambient_; 
    glm::vec3 diffuse_; 
    glm::vec3 specular_;


};

class DirLight:public Light{
public:
    DirLight(){ type_=LightType::Dirction; }
    DirLight(glm::vec3 dir,glm::vec3 am,glm::vec3 dif,glm::vec3 spe)
        :dir_(glm::normalize(dir)),Light(am,dif,spe){
        type_=LightType::Dirction; 
    }
    void getDirLight(glm::vec3& dir)override{
        dir=dir_;
    }

public:
    glm::vec3 dir_; // unit direction in real world

};

class PointLight:public Light{
    public:
    PointLight(){type_=LightType::Point; }
    PointLight(glm::vec3 p,glm::vec3 am,glm::vec3 dif,glm::vec3 spe):Light(am,dif,spe){
        type_=LightType::Point; 
    }
    void setAttenuation(float c,float li,float quad){
        constant_=c;
        linear_=li;
        quadratic_=quad;
    }
    void getPointLight(glm::vec3& pos,float& c,float& li,float& quad)override{
        pos=pos_;
        c=constant_;
        li=linear_;
        quad=quadratic_;
    }

public:
    glm::vec3 pos_;

    // attenuation=1.0/(constant+linear*dist+quadratic*dist*dist)
    float constant_;
    float linear_;
    float quadratic_;

};