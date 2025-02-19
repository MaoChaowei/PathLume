// this head file is for IO interaction between Render and Window instances.
#pragma once
#include"common/enumtypes.h"
#include"common/cputimer.h"
#include<string>

struct RenderSetting{
    bool show_tlas=false;
    bool show_blas=false;

    std::string scene_filename="";
    bool scene_change=false;       

    int bvh_leaf_num;
    bool leaf_num_change=false;

    ShaderType shader_type=ShaderType::Depth;
    bool shader_change=false;

    RasterizeType rasterize_type=RasterizeType::Naive;
    bool rasterize_change=false;
    
    bool back_culling=true;
    bool profile_report=true;

    // always true
    bool earlyz_test=true;
};

struct PerfCnt{
    int total_face_num_=0;
    int shaded_face_num_=0;
    int back_culled_face_num_=0;
    int clipped_face_num_=0;
    int hzb_culled_face_num_=0;
};

struct RenderIOInfo{
    RenderSetting setting_;
    PerfCnt profile_;
    CPUTimer timer_;
};