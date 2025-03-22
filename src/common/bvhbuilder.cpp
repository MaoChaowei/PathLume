#include"bvhbuilder.h"
#include"as.h"

BVHbuilder::BVHbuilder(std::shared_ptr<ObjectDesc> obj,uint32_t leaf_size):nodes_(std::make_unique<std::vector<BVHnode>>()){
    if(obj->getPrimitiveType()!=PrimitiveType::MESH){
        std::cerr<<"BVHbuilder:obj->getPrimitiveType()!=PrimitiveType::MESH!\n";
        exit(-1);
    }
    leaf_size_=leaf_size;
    const std::vector<Vertex>& vertices_=obj->getconstVertices();  
    const std::vector<uint32_t>& indices_=obj->getIndices();
    
    int facenum=indices_.size()/3;
    if(facenum<=0){
        std::cerr<<"BVHbuilder:facenum<=0!\n";
        exit(-1);
    }
    nodes_->reserve(2*facenum);          // reserve enough space to avoid frequent capacity expansion, which is super vital here!!
    pridices_.resize(facenum);
    for(int i=0;i<facenum;++i){
        pridices_[i]=i;
        AABB3d box(vertices_[indices_[i*3+0]].pos_,vertices_[indices_[i*3+1]].pos_,vertices_[indices_[i*3+2]].pos_);
        priboxes_.emplace_back(box);
    }

    buildBVH(0,facenum-1);
}

// building bvh tree for TLAS
BVHbuilder::BVHbuilder(const std::vector<std::shared_ptr<ASInstance>>& instances):nodes_(std::make_unique<std::vector<BVHnode>>()){
    leaf_size_=1;
    int num=instances.size();
    if(num<=0){
        std::cerr<<"BVHbuilder:instances.size()<=0!\n";
        exit(-1);
    }
    nodes_->reserve(num*2);          
    pridices_.resize(num);
    for(int i=0;i<num;++i){
        pridices_[i]=i;
        priboxes_.emplace_back(instances[i]->worldBBox_);
    }

    buildBVH(0,num-1);
}

// building implemention
int BVHbuilder::buildBVH(uint32_t start,uint32_t end){
    if(start>end) 
        return -1;

    // set current node
    nodes_->emplace_back();
    int nodeidx=nodes_->size()-1;
    auto& current=nodes_->at(nodeidx);
    current.primitive_num=end-start+1;
    current.prmitive_start=start;
    
    for(int i=start;i<=end;++i){
        current.bbox.expand(priboxes_[pridices_[i]]);
    }
    current.bbox.enlargeEpsilon();  // enlarge aabb box a little bit to avoid floating-error


    // leaf?
    if(end-start+1<=leaf_size_)
        return nodeidx;

    int lenx=current.bbox.length(0);
    int leny=current.bbox.length(1);
    int lenz=current.bbox.length(2);
    int axis=0;
    if(leny>lenx&&leny>lenz) axis=1;
    else if(lenz>lenx&&lenz>leny) axis=2;

    // sort to find the middle face
    sort(pridices_.begin()+start,pridices_.begin()+end+1,[this,axis](uint32_t a, uint32_t b){
        return this->cmp(a,b,axis);
    });
    uint32_t mid=(start+end)/2;

    // recursive 
    current.right=buildBVH(mid+1,end);
    current.left=buildBVH(start,mid);


    return nodeidx;
}

bool BVHbuilder::cmp(uint32_t a, uint32_t b,int axis){
    if(axis==0)
        return (priboxes_[a].min.x+priboxes_[a].max.x<priboxes_[b].min.x+priboxes_[b].max.x);
    else if(axis==1)
        return (priboxes_[a].min.y+priboxes_[a].max.y<priboxes_[b].min.y+priboxes_[b].max.y);
    else if(axis==2)
        return (priboxes_[a].min.z+priboxes_[a].max.z<priboxes_[b].min.z+priboxes_[b].max.z);
    
    return true;
}