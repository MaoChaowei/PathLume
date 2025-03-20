#pragma once
#include"common/common_include.h"

class Ray{
public:
    Ray(const glm::vec3 o,const glm::vec3 d,const float st=srender::EPSILON,const float ed=srender::MAXFLOAT):origin_(o),st_t_(st),dir_(d),ed_t_(ed){
        dir_=glm::normalize(dir_);
        inv_dir_=glm::vec3(1.0/dir_.x,1.0/dir_.y,1.0/dir_.z);
    }
    Ray(const Ray& ray){
        origin_=ray.origin_;
        dir_=ray.dir_;
        inv_dir_=ray.inv_dir_;
        st_t_=ray.st_t_;
        ed_t_=ray.ed_t_;
    }
    Ray& operator=(const Ray& ray){
        if(&ray==this)  return *this;
        origin_=ray.origin_;
        dir_=ray.dir_;
        inv_dir_=ray.inv_dir_;
        st_t_=ray.st_t_;
        ed_t_=ray.ed_t_;
        return *this;
    }

    glm::vec3 origin_;
    glm::vec3 dir_;
    glm::vec3 inv_dir_;
    float st_t_;
    float ed_t_;

    bool acceptT(float t)const{
        return t>st_t_&&t<ed_t_;
    }
};
