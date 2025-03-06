#pragma once
#include"common/common_include.h"

class Ray{
public:
    Ray(const glm::vec3 o,const glm::vec3 d,const float st=srender::EPSILON,const float ed=srender::MAXFLOAT):origin_(o),st_t_(st),dir_(d),ed_t_(ed){
        dir_=glm::normalize(dir_);
        inv_dir_=glm::vec3(1.0/dir_.x,1.0/dir_.y,1.0/dir_.z);
    }
    glm::vec3 origin_;
    glm::vec3 dir_;
    glm::vec3 inv_dir_;
    float st_t_;
    float ed_t_;
};
