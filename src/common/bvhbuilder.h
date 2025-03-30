#pragma once
#include "common/common_include.h"
#include "common/AABB.h"
#include"object.h"
#include"pathtracer/hitem.h"
#include<algorithm>

// forward declair
class ASInstance;

struct BVHnode:public Hitem
{
    int left;
    int right;

    AABB3d bbox;    // box in world or local space, pre-calculated when obj file is loaded.

    // specify which elements do this node control.
    unsigned int prmitive_start;
    unsigned int primitive_num ;

    BVHnode():left(-1),right(-1),prmitive_start(0),primitive_num(0){}

    /**
     * @brief the ray has an intersection with the aabb box only when tmin<tmax && tmax>0
     */
    bool anyHit(const Ray& ray)const override;

    bool rayIntersect(const Ray& ray,IntersectRecord& inst)const override{
        return anyHit(ray);
    }
};


class BVHbuilder
{
public:
    enum class BVHType{
        Normal,     // sort and pick midium as a partition
        SAH,        // use SAH tech to pick a partition
    };

    BVHbuilder()=delete;

    // building bvh tree for BLAS
    BVHbuilder(std::shared_ptr<ObjectDesc> obj,uint32_t leaf_size);

    // building bvh tree for TLAS
    BVHbuilder(const std::vector<std::shared_ptr<ASInstance>>& instances);

    // building implemention
    int buildBVH(uint32_t start,uint32_t end, BVHType type=BVHType::SAH);
    bool cmp(uint32_t a, uint32_t b,int axis);

    std::unique_ptr<std::vector<BVHnode>> moveNodes(){ return std::move(nodes_); }
    std::vector<uint32_t>& getPridices(){ return pridices_; }


public:

    std::unique_ptr<std::vector<BVHnode>> nodes_;        // root node is the nodes_[0].

    std::vector<uint32_t> pridices_;    // primitives_indices_
    std::vector<AABB3d> priboxes_;      // bbox for each element
    uint32_t leaf_size_=4;
};