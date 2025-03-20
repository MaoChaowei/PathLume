/* define all the implementation of the graph pipeline here */
#pragma once
#include"common/common_include.h"
#include"common/cputimer.h"
#include"softrender/shader.h"
#include"camera.h"
#include"scene_loader.h"
#include"buffer.h"
#include"hzb.h"
#include"common/AABB.h"
#include"common/utils.h"
#include"light.h"
#include"softrender/scanline.h"
#include"softrender/interface.h"
#include"window.h"
#include"pathtracer.h"
#include"film.h"

class Render{
public:

    Render();

    // MEMBER SETTING

    void setCamera(glm::vec3 pos,glm::vec3 lookat,glm::vec3 right,float fov=60,float ratio=1.0,int image_width=1024,float near=1.0,float far=1000.0);
    void afterCameraUpdate();

    void setBVHLeafSize(uint32_t num){scene_.setBVHsize(num);}
    void addObjInstance(std::string filename,glm::mat4& model,ShaderType shader,bool flipn=false,bool backculling=true);

    void setDeltaTime(float t){delta_time_=t;}

    void updateMatrix();
    void updateViewMatrix(){ mat_view_=camera_.getViewMatrix(); }
    void cleanFrame();

    Camera& getCamera(){ return camera_;}
    const ColorBuffer& getColorBuffer()const{ return *colorbuffer_;}
    const Scene& getScene()const{ return scene_;}

    // RASTERIZATION PIPELINE
    void pipelineInit();
    void pipelineBegin();

    void pipelineGeometryPhase();

    void pipelinePerInstance();
    void pipelineRasterizePhasePerInstance();

    void pipelineHZB_BVH();
    void pipelineRasterizePhaseHZB_BVH();
    
    void cullingTriangleInstance(ASInstance& instance,const glm::mat4 normal_mat);

    int pipelineClipping(std::vector<Vertex>& v,std::vector<Vertex>& out);
    void clipWithPlane(ClipPlane plane,std::vector<Vertex>&in,std::vector<Vertex>&out);
    bool backCulling(const glm::vec3& face_norm,const glm::vec3& dir)const;

    // PATH TRACING 
    void startPathTracer(){

        info_.pathtracer_timer_.clear();
        info_.pathtracer_timer_.start("001.PreProcess");

        // preprocess: 
        // 1.create film and tiles
        std::shared_ptr<Film> film=camera_.getNewFilm();
        film->initTiles(info_.tracer_setting_,colorbuffer_,&scene_);
        // 2.make sure: world position and emitters are prepared
        scene_.findAllEmitters();

        info_.pathtracer_timer_.stop("001.PreProcess");
        info_.pathtracer_timer_.start("002.Rendering");

        // rendering
        int thread_num=film->parallelTiles();

        info_.pathtracer_timer_.stop("002.Rendering");

        // write to file
        auto pathinfo=info_.tracer_setting_;

        colorbuffer_->saveToImage(pathinfo.filename_    \
                    +"_S"+std::to_string(pathinfo.spp_) \
                    +"_D"+std::to_string(pathinfo.max_depth_)   \
                    +"_T"+std::to_string(info_.pathtracer_timer_.getElapsedTime("002.Rendering"))   \
                    +"_C"+std::to_string(thread_num) \
                    +".png");
        
    }


    // INTERFACE
    void GameLoop();
    void handleKeyboardInput(int key, int action);
    void handleMouseInput(double xoffset, double yoffset);
    void moveCamera();

    // DEBUG
    void showTLAS();
    void showBLAS(const ASInstance& inst);
    void loadDemoScene(std::string name,ShaderType shader);
    void loadXMLfile(std::string name);
    void printProfile();


private:
    // rasterize
    void drawPoint(const glm::vec2 p, float radius,const glm::vec4 color);
    void drawLine(glm::vec2 t1,glm::vec2 t2);
    void drawLine3d(glm::vec3 t1,glm::vec3 t2,const glm::vec4& color=glm::vec4(255));
    void drawTriangleNaive();
    void drawTriangleHZB();
    void drawTriangleScanLine();

    void traverseBVHandDraw(const std::vector<BVHnode>& tree,uint32_t nodeIdx,bool is_TLAS,const glm::mat4& model=glm::mat4(1.0));
    void DfsTlas_BVHwithHZB(const std::vector<BVHnode>& tree,std::vector<AABB3d> &tlas_sboxes,const std::vector<ASInstance>& instances,uint32_t nodeIdx);
    void DfsBlas_BVHwithHZB(const ASInstance& inst,int32_t nodeIdx);


    void initRenderIoInfo();

private:
    bool is_init_=false;

    std::shared_ptr<Shader> sdptr_;
    std::shared_ptr<ScanLine::PerTriangleScanLiner> tri_scanliner_;
    Camera camera_;
    std::shared_ptr<ColorBuffer> colorbuffer_;
    std::shared_ptr<DepthBuffer> zbuffer_;
    std::shared_ptr<HZbuffer> hzb_;
    Scene scene_;
    
    glm::mat4 mat_view_;        // world to camera
    glm::mat4 mat_perspective_; // camera to clipspace
    glm::mat4 mat_viewport_;    // NDC to screen

    AABB2d box2d_;                // screen aabb box
    AABB3d box3d_;              

    bool resize_viewport_flag_=false;

public:
    // for ui
    float delta_time_;          // time spent to render last frame; (ms)
    bool keys_[1024]={0}; 
    
    RenderIOInfo info_;

    friend Window;
};