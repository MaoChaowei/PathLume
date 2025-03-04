#pragma once
#include"common/common_include.h"

class Ray{
public:
    Ray(const glm::vec3 o,const glm::vec3 d,const float st,const float ed):origin_(o),st_t_(st),dir_(d),ed_t_(ed){
        dir_=glm::normalize(dir_);
    }
    glm::vec3 origin_;
    float st_t_;
    glm::vec3 dir_;
    float ed_t_;
};
