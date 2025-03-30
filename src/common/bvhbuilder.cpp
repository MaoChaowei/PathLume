#include"bvhbuilder.h"
#include"as.h"

/**
 * @brief the ray has an intersection with the aabb box only when tmin<tmax && tmax>0
 */
bool BVHnode::anyHit(const Ray& ray)const{
    // set a infinite interval
    float interval_min=srender::MINFLOAT,interval_max=srender::MAXFLOAT;

    // for each axis, caculate the interval of t
    for(int i=0;i<3;++i){
        if(abs(ray.dir_[i])<srender::EPSILON){
            if(ray.origin_[i]<=bbox.min[i]||ray.origin_[i]>=bbox.max[i])
                return false;
        }
        else{
            float tmin=(bbox.min[i]-ray.origin_[i])*ray.inv_dir_[i];
            float tmax=(bbox.max[i]-ray.origin_[i])*ray.inv_dir_[i];
            if(tmin>tmax)
                std::swap(tmin,tmax);
            if(tmin>interval_min) interval_min=tmin;
            if(tmax<interval_max) interval_max=tmax;

            if(interval_min>=interval_max)
                return false;
        }
    }
    // the box is behind the ray!
    if(interval_max<0.f)
        return false;

    // // the ray need to accept this box
    // if(interval_max<ray.st_t_||interval_min>ray.ed_t_)
    //     return false;

    return true;
}



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
int BVHbuilder::buildBVH(uint32_t start,uint32_t end,BVHType type){
    if(start>end) 
        return -1;

    // set current node
    nodes_->emplace_back();
    int nodeidx=nodes_->size()-1;
    auto& current=nodes_->at(nodeidx);
    current.primitive_num=end-start+1;
    current.prmitive_start=start;
    current.left=current.right=-1;
    
    for(int i=start;i<=end;++i){
        current.bbox.expand(priboxes_[pridices_[i]]);
    }
    current.bbox.enlargeEpsilon(srender::AABBOX_EPS);  // enlarge aabb box a little bit to avoid floating-error


    // leaf?
    if(end-start+1<=leaf_size_)
        return nodeidx;

    int lenx=current.bbox.length(0);
    int leny=current.bbox.length(1);
    int lenz=current.bbox.length(2);
    int axis=0;
    if(leny>lenx&&leny>lenz) axis=1;
    else if(lenz>lenx&&lenz>leny) axis=2;

    /*----------------------- Partition ---------------------------*/
    uint32_t splitIdx=-1;
    if(type==BVHType::Normal){
        // sort to find the middle face
        sort(pridices_.begin()+start,pridices_.begin()+end+1,[this,axis](uint32_t a, uint32_t b){
            return this->cmp(a,b,axis);
        });
        splitIdx=(start+end)/2;
    }
    else if(type==BVHType::SAH){
        // ============================
        // 1. find the best partition axis 
        // ============================
        const float parentArea = current.bbox.boxSurfaceArea();
        float bestCost   = srender::MAXFLOAT;
        int   bestAxis   = -1;
        int   bestOffset = -1;

        for (int axis = 0; axis < 3; axis++)
        {
            std::sort(pridices_.begin() + start, pridices_.begin() + end + 1,
                    [this, axis](uint32_t a, uint32_t b)
                    {
                        const AABB3d& boxA = priboxes_[a];
                        const AABB3d& boxB = priboxes_[b];
                        float cA = (boxA.min[axis] + boxA.max[axis]); 
                        float cB = (boxB.min[axis] + boxB.max[axis]);
                        return cA < cB;
                    });

            // prefixBoxes[i] stands for aabb box of [start..i] 
            // suffixBoxes[i] stands for aabb box of [i..end]   
            const uint32_t n = end - start + 1;
            std::vector<AABB3d> prefixBoxes(n);
            std::vector<AABB3d> suffixBoxes(n);
            
            prefixBoxes[0] = priboxes_[pridices_[start]];
            for (uint32_t i = 1; i < n; i++) {
                prefixBoxes[i] = prefixBoxes[i - 1];
                prefixBoxes[i].expand(priboxes_[pridices_[start + i]]);
            }

            suffixBoxes[n - 1] = priboxes_[pridices_[start + (n - 1)]];
            for (int i = (int)n - 2; i >= 0; i--) {
                suffixBoxes[i] = suffixBoxes[i + 1];
                suffixBoxes[i].expand(priboxes_[pridices_[start + i]]);
            }

            // go through all possible partition position : [start..end)
            // left: [start..(start+i)] right: [(start+i+1)..end]
            for (uint32_t i = 0; i < n - 1; i++) 
            {
                float leftArea  = prefixBoxes[i].boxSurfaceArea();
                float rightArea = suffixBoxes[i + 1].boxSurfaceArea();
                uint32_t leftCount  = i + 1;       
                uint32_t rightCount = n - (i + 1); 

                float cost = leftArea * leftCount + rightArea * rightCount;

                if (cost < bestCost) {
                    bestCost   = cost;
                    bestAxis   = axis;
                    bestOffset = i;
                }
            }
        }

        // if fail to find a good partion ( many boxes with Overlapping centers ), regard them as one node
        if (bestAxis < 0) {
            return nodeidx;
        }

        // ============================
        // 2. sort again with the optical axis
        // ============================
        std::sort(pridices_.begin() + start, pridices_.begin() + end + 1,
                [this, bestAxis](uint32_t a, uint32_t b)
                {
                    const AABB3d& boxA = priboxes_[a];
                    const AABB3d& boxB = priboxes_[b];
                    float cA = (boxA.min[bestAxis] + boxA.max[bestAxis]) * 0.5f;
                    float cB = (boxB.min[bestAxis] + boxB.max[bestAxis]) * 0.5f;
                    return cA < cB;
                });
        
        splitIdx = start + bestOffset; // left [start..splitIndex], right[splitIndex+1..end]

    }
    else{
        throw std::runtime_error("BVHbuilder::buildBVH-> unknown BVHType!");
    }


    // recursive 
    current.right=buildBVH(splitIdx+1,end);
    current.left=buildBVH(start,splitIdx);


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