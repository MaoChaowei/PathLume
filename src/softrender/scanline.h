#pragma once
#include"common/common_include.h"
#include"common/AABB.h"
#include"object.h"
#include"buffer.h"
#include"shader.h"
#include<algorithm>

namespace ScanLine{

struct EdgeNode{
    float x_;               // upper endpoint's x value as initial value. when y--,then x--.
    float dx_dy_;           // -dx/dy
    int yspan_;        // y span of the edge

    /* ----- for interpolation ------- */
    // z-test
    float inv_z_;           // 1/z in screen space
    float dinvz_dy_;        // when dy--, inv_z+=dinvy_dz
    // attribute 
    float inv_w_;           // 1/w in clip space = -1/z in view space
    float dinvw_dy_;
    glm::vec3 norm_w_;      // normal in world space
    glm::vec3 dnorm_w_dy_;
    glm::vec4 color_w_;     // color
    glm::vec4 dcol_w_dy_;
    glm::vec2 uv_w_;        // uv
    glm::vec2 duv_w_dy_;
    glm::vec3 wpos_w_;      // world pos
    glm::vec3 dwpos_w_dy_;

    bool init_step_y_;
    // bool horizontal_;
    const Vertex*  top_vertex_; 
    const Vertex*  down_vertex_; 

    EdgeNode(const Vertex* top,const Vertex* down,bool step);

    void normalizeAttributes();

    void stepY();

};

struct PrimitiveNode{
    int yspan_;
    
    std::shared_ptr<ObjectDesc> obj_;
    uint32_t pri_idx_;

    std::vector<std::shared_ptr<EdgeNode>> all_edges_;
};

// encapulate all methods for scan-line conversion algorithm
class ScanLiner{
public:

protected:
    bool init_=false;
    std::vector<PrimitiveNode> PT_;
    std::vector<EdgeNode> ET_;
    std::vector<std::vector<uint32_t>> idx2ET_; // Y->ET indices
    std::vector<std::vector<uint32_t>> idx2PT_;//  Y->PT indices

    std::vector<PrimitiveNode*> APT_;
    std::vector<EdgeNode*> AET_;

    // buffers will not be cleared but updated directly. if we need to clean, clean them outside. 
    std::shared_ptr<ColorBuffer> fbuffer_;
    std::shared_ptr<DepthBuffer> zbuffer_;

    AABB2d screen_box_;

};



class PerTriangleScanLiner:public ScanLiner{
public:
    PerTriangleScanLiner(const AABB2d& box,std::shared_ptr<ColorBuffer>& fb,std::shared_ptr<DepthBuffer>& zb,std::shared_ptr<Shader>& sp);

    void reFresh();

    void scanConvert(std::vector<const Vertex*>& vs);

    void rasterizeScanline(int y,const EdgeNode* left,const EdgeNode* right);

    bool buildET(std::vector<const Vertex*>& vs);

private:
    std::shared_ptr<Shader> sdptr_;

};

}