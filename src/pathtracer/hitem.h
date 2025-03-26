#pragma once
#include"ray.h"
#include"vertex.h"
#include"material.h"
#include"enumtypes.h"

// forward declaration
class BSDF;

/**
 * @brief record necessary information of the hit point,
 *  including position, distance, normal, TBN matrix...
 * 
 */
class IntersectRecord{
public:
    IntersectRecord():pos_(0.0),t_(srender::MAXFLOAT),normal_(0.0),uv_(-1.f),material_(nullptr),bvhnode_idx_(-1){}

    IntersectRecord& operator=(const IntersectRecord& inst);

    // use material to initialize bsdf
    std::shared_ptr<BSDF> getBSDF();

    // Generate an orthonormal base for tangent space samples. Reference: https://graphics.pixar.com/library/OrthonormalB/paper.pdf
    std::shared_ptr<glm::mat3> genTBN();

    // transform ray from world space to tangent space local to the hit point.
    glm::vec3 ray2TangentSpace(const glm::vec3& world_dir);

    // transform the sampled Wi from tangent space to world space.
    glm::vec3 wi2WorldSpace(const glm::vec3& wi);

public:

    glm::vec3 pos_;
    float t_;           // distance from origin to the hit point
    glm::vec3 normal_;  // the front direction of the face,normalized
    glm::vec2 uv_;
    std::shared_ptr<glm::mat3> TBN_;     // Tangent, Bitangent and Normal vectors in world space

    std::shared_ptr<const Material> material_;  // to generate bsdf

    int32_t bvhnode_idx_;
    
};

/**
 * @brief Hitem, aka Hitable item, works as the base class for all hitable things,
 * such as triangle,sphere and so on
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
     * @brief Moller Trumbore intersection algorithm
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

        if(b1<0||b2<0||1-b1-b2<0)   // give a -eps tolerance?
            return false;

        if(ray.acceptT(t)){
            // record the nearest hit.
            if(inst.t_>t){
                inst.pos_=ray.origin_+t*ray.dir_;
                inst.t_=t;

                // interpolate Shading Normal
                auto local_norm=(1-b1-b2)*points[0]->norm_+b1*points[1]->norm_+b2*points[2]->norm_; // this is always towards the front face of mesh
                if(glm::dot(local_norm,ray.dir_)>0.0)   // ray comes from the back face.
                    local_norm=-local_norm; // reverse the norm of intersection

                inst.normal_=glm::normalize(local_norm);

                // interpolate UV
                for(int i=0;i<2;++i){
                    inst.uv_[i]=(1-b1-b2)*points[0]->uv_[i]+b1*points[1]->uv_[i]+b2*points[2]->uv_[i]; 
                }

                inst.material_=material_;
                
            }
            return true;
        }
        else   return false;

    }

private:
    std::vector<const Vertex*> points;
    std::shared_ptr<const Material> material_;
    
};