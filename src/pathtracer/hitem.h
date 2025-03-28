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

    // Use material to initialize bsdf, and then Select a bsdf with random number u(in [0,1))
    std::shared_ptr<BSDF> getBSDF(float u);

    // Generate an orthonormal base for tangent space samples. Reference: https://graphics.pixar.com/library/OrthonormalB/paper.pdf
    std::shared_ptr<glm::mat3> genTBN();

    // transform ray from world space to tangent space local to the hit point.
    glm::vec3 ray2TangentSpace(const glm::vec3& world_dir);

    // transform the sampled Wi from tangent space to world space.
    glm::vec3 wi2WorldSpace(const glm::vec3& wi);

public:

    glm::vec3 pos_;
    float t_;           // distance from origin to the hit point
    glm::vec3 normal_;  // the Shading normal of the face,normalized
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
    bool anyHit(const Ray& ray)const override;

    /**
     * @brief Moller Trumbore intersection algorithm
     * @param ray :must has been transformed into triangle's model space
     * @param inst : the record are all in model space
     * @return true : do have a intersection
     */
    bool rayIntersect(const Ray& ray,IntersectRecord& inst)const override;

private:
    std::vector<const Vertex*> points;
    std::shared_ptr<const Material> material_;
    
};