// this head file is for IO interaction between Render and Window instances.
#pragma once
#include"common/enumtypes.h"
#include"common/cputimer.h"
#include<string>
#include<mutex>

struct RasterSetting{

    bool show_tlas=false;
    bool show_blas=false;

    // std::string scene_filename="";
    bool scene_change=false;       

    int bvh_leaf_num;
    bool leaf_num_change=false;

    ShaderType shader_type=ShaderType::Depth;
    bool shader_change=false;

    RasterizeType rasterize_type=RasterizeType::Naive;
    bool rasterize_change=false;
    
    bool back_culling=true;

    // always true
    bool earlyz_test=true;
};

struct RTracingSetting{
    uint32_t max_depth_;
    uint32_t tiles_num_;
    uint32_t spp_;
    uint32_t light_split_;

    // std::string filepath_;
    // std::string filename_;
    float render_time_;
    // havn't supported yet
    std::string sampler_;
};

struct PerfCnt{
    // rasterizer
    int total_face_num_=0;
    int shaded_face_num_=0;
    int back_culled_face_num_=0;
    int clipped_face_num_=0;
    int hzb_culled_face_num_=0;

    // path tracer


    void clear(){
        total_face_num_=0;
        shaded_face_num_=0;
        back_culled_face_num_=0;
        clipped_face_num_=0;
        hzb_culled_face_num_=0;
    }
};

struct RenderIOInfo{
    /*------------thread unsafe-------------*/
    
    std::mutex mx_msg_;             
    bool begin_path_tracing=false;  // trigger render to path tracing work mode.
    bool end_path_tracing=true;

    /*------------thread safe-------------*/
    bool profile_report=true;

    std::string filename_;
    RasterSetting raster_setting_;
    RTracingSetting tracer_setting_;
    PerfCnt profile_;
    CPUTimer rasterize_timer_;            // (us)
    
};