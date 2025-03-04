#pragma once
#include"pathtracer/ray.h"

class Hitem;
struct IntersectRecord{
    glm::vec3 wpos_;
    const Hitem* face_ptr_;
    
};

/**
 * @brief Hitem, aka Hitable item, works as base class for all hitable things, such as triangle,sphere and so on
 * 
 */
class Hitem{
public:
    // simply to trace a shadow ray
    virtual bool anyHit(const Ray& ray)=0;
    virtual void rayIntersect(const Ray& ray,IntersectRecord& inst)=0;
private:


};