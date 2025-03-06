#pragma once
#include"pathtracer/ray.h"
#include"vertex.h"
#include"material.h"

class Hitem;

struct IntersectRecord{
    glm::vec3 pos_;
    float t_;           // distance from origin to the hit point
    glm::vec3 normal_;  // always opposed to incident ray
    glm::vec3 wo_;      // revered incident ray direction
    std::shared_ptr<const Material> material_;
    struct{
        uint32_t tlas_node_idx_;
        uint32_t blas_node_idx_;
    }as_node_;

    IntersectRecord& operator=(const IntersectRecord& inst){
        if(this==&inst)
            return *this;
        pos_=inst.pos_;
        t_=inst.t_;
        normal_=inst.normal_;
        wo_=inst.wo_;
        material_=inst.material_;
        as_node_=inst.as_node_;
        return *this;
    }
};

/**
 * @brief Hitem, aka Hitable item, works as base class for all hitable things, such as triangle,sphere and so on
 * 
 */
class Hitem{
public:
    virtual ~Hitem(){}
    // simply to trace a shadow ray
    virtual bool anyHit(const Ray& ray)const=0;
    virtual bool rayIntersect(const Ray& ray,IntersectRecord& inst)const=0;
private:


};


class Htriangle:public Hitem{
public:
    Htriangle(const Vertex* a,const Vertex* b,const Vertex* c,std::shared_ptr<const Material> mat):material_(mat){
        points.emplace_back(a);
        points.emplace_back(b);
        points.emplace_back(c);
    }
    bool anyHit(const Ray& ray)const override{
        glm::vec3 e1=points[1]->pos_-points[0]->pos_;
        glm::vec3 e2=points[2]->pos_-points[0]->pos_;
        glm::vec3 s=ray.origin_-points[0]->pos_;
        glm::vec3 s1=glm::cross(ray.dir_,e2);
        glm::vec3 s2=glm::cross(s,e1);

        float det=glm::dot(s1,e1);
        // parallel or degeneration of triangle
        if(fabs(det)<srender::EPSILON) 
            return false;

        float factor=1.0/det;

        float t=factor*glm::dot(s2,e2);
        // only keep the forward hit
        if(t<0.f)   
            return false;

        float b1=factor*glm::dot(s1,s);
        float b2=factor*glm::dot(s2,ray.dir_);

        if(b1<-srender::EPSILON||b2<-srender::EPSILON||1-b1-b2<-srender::EPSILON)
            return false;

        return true;
    }

    /**
     * @brief Moller Trumbore
     * @param ray :must has been transformed into triangle's model space
     * @param inst : the record are all in model space
     * @return true : do have a intersection
     */
    bool rayIntersect(const Ray& ray,IntersectRecord& inst)const override{
        assert(fabs(glm::length(ray.dir_)-1.0)<1e-6);

        glm::vec3 e1=points[1]->pos_-points[0]->pos_;
        glm::vec3 e2=points[2]->pos_-points[0]->pos_;
        glm::vec3 s=ray.origin_-points[0]->pos_;
        glm::vec3 s1=glm::cross(ray.dir_,e2);
        glm::vec3 s2=glm::cross(s,e1);

        float det=glm::dot(s1,e1);
        // parallel or degeneration of triangle
        if(fabs(det)<srender::EPSILON) 
            return false;

        float factor=1.0/det;

        float t=factor*glm::dot(s2,e2);
        // only keep the forward hit
        if(t<0.f)   
            return false;

        float b1=factor*glm::dot(s1,s);
        float b2=factor*glm::dot(s2,ray.dir_);

        if(b1<-srender::EPSILON||b2<-srender::EPSILON||1-b1-b2<-srender::EPSILON)
            return false;

        // record the nearest hit.
        if(inst.t_>t){
            inst.pos_=ray.origin_+t*ray.dir_;
            inst.t_=t;
            inst.wo_=-ray.dir_;

            auto local_norm=(1-b1-b2)*points[0]->norm_+b1*points[1]->norm_+b2*points[2]->norm_; // this is always towards the front face of mesh
            if(glm::dot(local_norm,ray.dir_)>0.0)   // ray comes from the back face.
                local_norm=-local_norm; // reverse
            inst.normal_=glm::normalize(local_norm);

            inst.material_=material_;
        }

        return true;
    }

private:
    std::vector<const Vertex*> points;
    std::shared_ptr<const Material> material_;
    
};