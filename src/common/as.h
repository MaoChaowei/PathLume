#pragma once
#include "common/common_include.h"
#include "common/utils.h"
#include "bvhbuilder.h"
#include"softrender/shader.h"
#include"pathtracer/hitem.h"


class AccelStruct{
public:
    AccelStruct():tree_(std::make_unique<std::vector<BVHnode>>()){}
    virtual ~AccelStruct(){}

    virtual bool traceRayInDetail(const Ray& ray,IntersectRecord& inst)const=0;
    bool traceRayInAccel(const Ray& ray,int32_t node_idx,IntersectRecord& inst,bool is_tlas)const;

    std::unique_ptr<std::vector<BVHnode>> tree_;                  

};

class BLAS:public AccelStruct
{
public:
    BLAS()=delete;
    BLAS(std::shared_ptr<ObjectDesc> obj,uint32_t leaf_size){
        // bind object
        object_ = obj;
        // build its bvh
        BVHbuilder builder(obj,leaf_size);
        tree_=builder.moveNodes();
        primitives_indices_=std::make_unique<std::vector<uint32_t>>(std::move(builder.getPridices()));
    }
    bool traceRayInDetail(const Ray& ray,IntersectRecord& inst)const override;

    
public:
    std::shared_ptr<ObjectDesc> object_;
    std::unique_ptr<std::vector<uint32_t>> primitives_indices_;     // BVHnode-->primitives_indices_-->object_'s face/primitive
};

struct PrimitiveHolder{         // because of the neccessity of clipping, each frame updates all the primitives of the instance.
    ClipFlag clipflag_;         // 0: accepted; 1: clipped; 2: refused;
    int32_t mtlidx_;            // point to its material in blas_
    int32_t vertex_start_pos_;  // point to vertices_
    int32_t vertex_num_;        // specify the range starting from vertex_start_pos_

    PrimitiveHolder(ClipFlag cf, int32_t mtl, int32_t startpos, int32_t num)
    : clipflag_(cf),
        mtlidx_(mtl),
        vertex_start_pos_(startpos),
        vertex_num_(num)
    {}
};

class ASInstance{
public:
    ASInstance(std::shared_ptr<BLAS>blas,const glm::mat4& mat,ShaderType shader);

    void refreshVertices();

    void BLASupdateSBox();

    void updateScreenBox(int32_t node_idx,std::vector<BVHnode>&blas_tree,std::vector<uint32_t>& primitive_indices);



public:
    std::shared_ptr<BLAS> blas_;

    // preserve properties for each frame after clipping.
    std::unique_ptr<std::vector<Vertex>> vertices_;
    std::unique_ptr<std::vector<PrimitiveHolder>> primitives_buffer_;
    std::unique_ptr<std::vector<AABB3d>> blas_sboxes_;

    glm::mat4 modle_;
    glm::mat4 inv_modle_;
    AABB3d worldBBox_;
    ShaderType shader_;
};

class TLAS: public AccelStruct
{
public:
    TLAS():tlas_sboxes_(std::make_unique<std::vector<AABB3d>>()){}

    void buildTLAS();

    void TLASupdateSBox();

    void updateScreenBox(int32_t node_idx);

    bool traceRayInDetail(const Ray& ray,IntersectRecord& inst)const override;

public:
    std::vector<std::shared_ptr<ASInstance>> all_instances_;    // BVHnode-->isntances
    std::unique_ptr<std::vector<AABB3d>> tlas_sboxes_;  

};