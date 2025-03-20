#include"render.h"
#include"window.h"
#include"thread"

void Render::GameLoop(){


    // init glfw window and glad
    int width=camera_.getImageWidth();
    int height=camera_.getImageHeight();

    Window window;
    window.init("PathLume", 0.8, 0.9);
    window.bindRender(this);
    window.bindRenderIOInfo(&info_);// shared memory

    int cnt=0;
    auto lastTime=std::chrono::high_resolution_clock::now();
    auto curTime=lastTime;

    static bool during_path_tracing=false;  // trigger pathtracing only once
    while (!window.shouldClose()) {
        // Processing Input
        window.processInput();

        // Start imGui for this frame

        window.newImGuiFrame(); 

        if(!info_.begin_path_tracing){
             // move camera according to the input
            this->moveCamera();

            // clean last frame
            this->cleanFrame();

            // the pipeline goes well here
            this->pipelineBegin();

            during_path_tracing=false;
        }
        else if(!during_path_tracing){
            std::thread rtwork(&Render::startPathTracer,this);
            rtwork.detach();
            during_path_tracing=true;
        }

        // update frameBuffer
        window.updateFrame(colorbuffer_->getAddr());

        
        curTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - lastTime).count();
        // std::cout<<"frame : "<<cnt++<<" ,duration:"<<duration <<" ms "<<std::endl;

        // render imGui for this frame
        window.renderImGuiFrame();

        // postrender events
        window.swapBuffer();

        lastTime=curTime;
        this->setDeltaTime(float(duration));


// #ifdef TIME_RECORD
//         this->timer_.report();
// #endif
    }// game loop

    window.shutdownImGui();
    glfwTerminate();

}


void Render::handleKeyboardInput(int key, int action) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
        keys_[key]=true;
    else if(action==GLFW_RELEASE)
        keys_[key]=false;
}

void Render::moveCamera(){
    if (keys_[GLFW_KEY_W]) camera_.processKeyboard(CameraMovement::FORWARD,delta_time_);
    if (keys_[GLFW_KEY_S]) camera_.processKeyboard(CameraMovement::BACKWARD,delta_time_);
    if (keys_[GLFW_KEY_A]) camera_.processKeyboard(CameraMovement::LEFT,delta_time_);
    if (keys_[GLFW_KEY_D]) camera_.processKeyboard(CameraMovement::RIGHT,delta_time_);
    if (keys_[GLFW_KEY_TAB]) camera_.processKeyboard(CameraMovement::REFRESH,delta_time_);
}

void Render::handleMouseInput(double xoffset, double yoffset) {
    camera_.processMouseMovement(static_cast<float>(xoffset), static_cast<float>(yoffset));
}
