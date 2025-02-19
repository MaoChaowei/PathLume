#pragma once
#include"common/common_include.h"
#include"common/AABB.h"
#include"buffer.h"

class HZbuffer{
public:
    HZbuffer(){};
    HZbuffer(uint32_t w,uint32_t h);

    void updateDepth(uint32_t x,uint32_t y,float depth);

    // Refuse a 3d-box in screen space.
    bool rapidRefuseBox(const AABB3d& box);

    // map coarsest_length from the coarsest level to a higher level, where there are 4 depthes at most covering coarsest_length.
    int getLevel(float coarsest_length);

    bool finestZTest(uint32_t x,uint32_t y, float new_depth);
    void clear();

    DepthBuffer& getFinesetZbuffer(){return hzb_[fineset_level_];}

private:
    uint32_t width_;
    uint32_t height_;
    uint32_t fineset_level_;
    std::vector<DepthBuffer> hzb_;
    
};