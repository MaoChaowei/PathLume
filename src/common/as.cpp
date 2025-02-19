#include"as.h"


ASInstance::ASInstance(std::shared_ptr<BLAS>blas,const glm::mat4& mat,ShaderType shader):blas_(blas),modle_(mat),shader_(shader){
        AABB3d rootBox=blas_->tree_->at(0).bbox;
        worldBBox_=rootBox.transform(modle_);

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
    }
    tlas_sboxes_->resize(tree_->size());
}

void TLAS::TLASupdateSBox(){
    for(auto& inst:all_instances_){
        inst.BLASupdateSBox();
        auto& blas_sbox=inst.blas_sboxes_->at(0);
    }
    updateScreenBox(0);
}


void TLAS::updateScreenBox(int32_t node_idx){
    
    int32_t left_idx=tree_->at(node_idx).left;
    int32_t right_idx=tree_->at(node_idx).right;
    auto& sbox=tlas_sboxes_->at(node_idx);

    if(left_idx==-1&&right_idx==-1){
        int st=tree_->at(node_idx).prmitive_start;
        sbox=all_instances_[st].blas_sboxes_->at(0);
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
    
