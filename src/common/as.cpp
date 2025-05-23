#include"as.h"
#include"hitem.h"

/**
* @brief trace a ray in the bvh acceleration struct
* 
* @param ray : ray must in bvh's space(tlas: world space; blas: model space)
* @param node_idx : the current node
* @return true : found a hit
*/
bool AccelStruct::traceRayInAccel(const Ray& ray,int32_t node_idx,IntersectRecord& inst,bool is_tlas)const{
   assert(node_idx>=0&&node_idx<tree_->size());
    // optimize direction: put box hit test outside as a sort of guidance
   if(!tree_->at(node_idx).anyHit(ray))
       return false;
    
    auto right=tree_->at(node_idx).right;
    auto left=tree_->at(node_idx).left;

    // reach leaf node, go down to next level
    if(left==-1&&right==-1){

        inst.bvhnode_idx_=node_idx;
        return traceRayInDetail(ray,inst);
    }
    assert(left!=-1&&right!=-1);

    IntersectRecord left_hit,right_hit;

    
    bool flag_lt=traceRayInAccel(ray,left,left_hit,is_tlas);
    bool flag_rt=traceRayInAccel(ray,right,right_hit,is_tlas);

    // if(flag_lt&&left_hit.t_<right_hit.t_){
    //     inst.t_=left_hit.t_;
    //     inst.normal_=left_hit.normal_;
    //     inst.material_=left_hit.material_;
    //     inst.pos_=left_hit.pos_;
    // }else if(flag_rt&&right_hit.t_<left_hit.t_){
    //     inst.t_=right_hit.t_;
    //     inst.normal_=right_hit.normal_;
    //     inst.material_=right_hit.material_;
    //     inst.pos_=right_hit.pos_;
    // }
    // else{
    //     return false;
    // }
    // return true;  

    if(flag_lt||flag_rt){
        inst=left_hit.t_<right_hit.t_?left_hit:right_hit;
        return true;
    }
    else
        return false;
    
}

/**
 * @brief BLAS carries on a hit test among all the triangles inside a box(i.e. bvh-leaf)
 * @param inst: record the nearest hit
 * @return true : do get a hit
 */
bool BLAS::traceRayInDetail(const Ray& ray,IntersectRecord& inst)const{

    bool hitted=false;

    int id=inst.bvhnode_idx_;
    BVHnode& node=tree_->at(id);
    auto& vertices=object_->getVertices();
    auto& indices=object_->getIndices();

    // check all the primitives inside
    for(int i=0;i<node.primitive_num;++i){
        // construct Hitem
        std::vector<const Vertex*> temp;
        auto idx=primitives_indices_->at(node.prmitive_start+i);
        for(int j=0;j<3;++j){
            temp.push_back(&vertices[indices[idx*3+j]]);
        }
        Htriangle tri(temp[0],temp[1],temp[2],object_->getFaceMtl(idx));

        // record the nearest hit
        if(tri.rayIntersect(ray,inst))
            hitted=true;
    }

    return hitted;

}

/**
 * @brief tranform ray into instance's model world, and continue to trace ray in blas.
 * 
 */
bool TLAS::traceRayInDetail(const Ray& ray,IntersectRecord& inst)const{
    // find asinstance
    auto& node=tree_->at(inst.bvhnode_idx_);
    auto& instance=*all_instances_.at(node.prmitive_start);

    // transform ray into model's space
    auto mat_inv=instance.inv_modle_;
    auto morigin=mat_inv*glm::vec4(ray.origin_,1.0);
    auto mdir=mat_inv*glm::vec4(ray.dir_,0.0);
    Ray mray(morigin,mdir);

    // dive into blas
    if(instance.blas_->traceRayInAccel(mray,0,inst,false)){
        // transform intersect record back to world space.
        inst.pos_=instance.modle_*glm::vec4(inst.pos_,1.0);
        inst.normal_=glm::normalize(glm::vec3(glm::transpose(mat_inv)*glm::vec4(inst.normal_,0.0)));
        inst.t_=glm::length(inst.pos_-ray.origin_);
        
        return true;
    }

    return false;
}

ASInstance::ASInstance(std::shared_ptr<BLAS>blas,const glm::mat4& mat,ShaderType shader):blas_(blas),modle_(mat),shader_(shader){
    AABB3d rootBox=blas_->tree_->at(0).bbox;
    worldBBox_=rootBox.transform(modle_);

    inv_modle_=glm::inverse(modle_);

    vertices_=std::make_unique<std::vector<Vertex>>();
    primitives_buffer_=std::make_unique<std::vector<PrimitiveHolder>>();
    blas_sboxes_=std::make_unique<std::vector<AABB3d>>(blas_->tree_->size());
}

void ASInstance::refreshVertices(){
    vertices_=std::make_unique<std::vector<Vertex>>();
    primitives_buffer_=std::make_unique<std::vector<PrimitiveHolder>>();
    blas_sboxes_=std::make_unique<std::vector<AABB3d>>(blas_->tree_->size());
}

void ASInstance::BLASupdateSBox(){
    auto& blas_tree=*blas_->tree_;
    auto& primitive_indices=*blas_->primitives_indices_;
    updateScreenBox(0,blas_tree,primitive_indices);
}

void ASInstance::updateScreenBox(int32_t node_idx,std::vector<BVHnode>&blas_tree,std::vector<uint32_t>& primitive_indices){

    int32_t left_idx=blas_tree[node_idx].left;
    int32_t right_idx=blas_tree[node_idx].right;

    auto& sbox=blas_sboxes_->at(node_idx);
    sbox.reset();

    if(left_idx==-1&&right_idx==-1){

        int st_primitive=blas_tree[node_idx].prmitive_start;

        for(int i=0;i<blas_tree[node_idx].primitive_num;++i){

            uint32_t idx=primitive_indices[st_primitive+i];

            auto cflag=primitives_buffer_->at(idx).clipflag_;
            if(cflag==ClipFlag::accecpted||cflag==ClipFlag::clipped){
                int32_t st_ver=primitives_buffer_->at(idx).vertex_start_pos_;

                for(int v=0;v<primitives_buffer_->at(idx).vertex_num_;++v){ // >=3 vertivces
                    sbox.addPoint(this->vertices_->at(st_ver+v).s_pos_);
                }
            }
        }
        return;
    }

    if(left_idx!=-1){
        updateScreenBox(left_idx,blas_tree,primitive_indices);
    }
    if(right_idx!=-1){
        updateScreenBox(right_idx,blas_tree,primitive_indices);
    }

    sbox.expand(blas_sboxes_->at(left_idx));
    sbox.expand(blas_sboxes_->at(right_idx));

    return;
}



void TLAS::buildTLAS(){

    if(all_instances_.size()){
        BVHbuilder builder(all_instances_);
        tree_=builder.moveNodes();
        std::vector<uint32_t> element_indices_=builder.getPridices();

        // sort all_instances by the order in element_indices
        // FROM: bvhnodeIdx-->element_indices_-->all_instances_
        // TO:   bvhnodeIdx-->all_instances_
        assert(element_indices_.size()==all_instances_.size());
        std::vector<std::shared_ptr<ASInstance>> temp;
        for(int i=0;i<element_indices_.size();++i){
            temp.emplace_back(all_instances_[element_indices_[i]]);
        }
        all_instances_=temp;
    }

    tlas_sboxes_->resize(tree_->size());
}

void TLAS::TLASupdateSBox(){
    for(auto& inst:all_instances_){
        inst->BLASupdateSBox();
        auto& blas_sbox=inst->blas_sboxes_->at(0);
    }
    updateScreenBox(0);
}


void TLAS::updateScreenBox(int32_t node_idx){
    
    int32_t left_idx=tree_->at(node_idx).left;
    int32_t right_idx=tree_->at(node_idx).right;
    auto& sbox=tlas_sboxes_->at(node_idx);

    if(left_idx==-1&&right_idx==-1){
        int st=tree_->at(node_idx).prmitive_start;
        auto& instance=*all_instances_.at(st);
        sbox=instance.blas_sboxes_->at(0);
        return;
    }

    if(left_idx!=-1){
        updateScreenBox(left_idx);
    }
    if(right_idx!=-1){
        updateScreenBox(right_idx);
    }

    sbox.reset();
    sbox.expand(tlas_sboxes_->at(left_idx));
    sbox.expand(tlas_sboxes_->at(right_idx));

    return;
}
    
