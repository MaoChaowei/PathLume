#include"hzb.h"

HZbuffer::HZbuffer(uint32_t w,uint32_t h){
    assert(w%2==0&&h%2==0);
    width_=w;
    height_=h;
    fineset_level_=std::log2(std::min((int)width_,(int)height_))-1;
    hzb_.resize(fineset_level_+1);
    for(int i=fineset_level_;i>=0;--i){
        hzb_[i].reSetBuffer(w,h);
        w/=2;
        h/=2;
    }
}

void HZbuffer::updateDepth(uint32_t x,uint32_t y,float depth){

    hzb_[fineset_level_].setDepth(x,y,depth);

    for(int i = fineset_level_ - 1; i >= 0; --i){
        uint32_t parent_x = x / 2;
        uint32_t parent_y = y / 2;

        float current_parent_depth = hzb_[i].getDepth(parent_x, parent_y);

        // son depthes
        float child_depth1 = hzb_[i + 1].getDepth(parent_x * 2, parent_y * 2);
        float child_depth2 = hzb_[i + 1].getDepth(parent_x * 2 + 1, parent_y * 2);
        float child_depth3 = hzb_[i + 1].getDepth(parent_x * 2, parent_y * 2 + 1);
        float child_depth4 = hzb_[i + 1].getDepth(parent_x * 2 + 1, parent_y * 2 + 1);

        float new_parent_depth = std::max({child_depth1, child_depth2, child_depth3, child_depth4});

        if(new_parent_depth != current_parent_depth){
            hzb_[i].setDepth(parent_x, parent_y, new_parent_depth);
        }
        else{
            break; 
        }

        x = parent_x;
        y = parent_y;
    }
}

// Refuse a 3d-box in screen space.
bool HZbuffer::rapidRefuseBox(const AABB3d& box){
    AABB3d illegle_box;
    if(box.min==illegle_box.min&&box.max==illegle_box.max){
        return true;
    }
    
    float nearest_depth=box.min.z;

    float box_length=std::max(box.max.y-box.min.y,box.max.x-box.min.x);
    int cur_level=fineset_level_-getLevel(box_length);
    float inv_unit=1.0/(std::pow(2,fineset_level_-cur_level));

    int left=std::floor(box.min.x*inv_unit);
    int right=std::floor(box.max.x*inv_unit);
    int top=std::floor(box.max.y*inv_unit);
    int down=std::floor(box.min.y*inv_unit);

    for(int r=top;r>=std::max(0,down);--r){
        for(int c=std::max(0,left);c<=right;++c){
            float depth=hzb_[cur_level].getDepth(c,r);
            // if this quarter is closer then we can't refuse this box
            if(depth>nearest_depth){
                return false;
            }
        }
    }

    // yay we can refuse this!
    return true;
}

// map coarsest_length from the coarsest level to a higher level, where there are 4 depthes at most covering coarsest_length.
int HZbuffer::getLevel(float coarsest_length){
    return std::min((int)std::ceil(std::log2(std::max(coarsest_length,1.f))),(int)fineset_level_);
}

bool HZbuffer::finestZTest(uint32_t x,uint32_t y, float new_depth){
    if(hzb_[fineset_level_].zTest(x,y,new_depth)==true){
        updateDepth(x,y,new_depth);
        return true;
    }
    return false;
}

void HZbuffer::clear(){
    for(auto& buffer:hzb_){
        buffer.clear();
    }
}