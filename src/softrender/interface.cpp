#include"render.h"
#include"window.h"


void Render::GameLoop(){


    // init glfw window and glad
    int width=camera_.getImageWidth();
    int height=camera_.getImageHeight();

    window_.init("PathLume", 1400, 1200);
    window_.bindRender(this);
    window_.bindRenderIOInfo(&info_);

    int cnt=0;
    auto lastTime=std::chrono::high_resolution_clock::now();
    auto curTime=lastTime;

    while (!window_.shouldClose()) {
        // Processing Input
        window_.processInput();
        if(resize_viewport_flag_){
            window_.resizeViewport(camera_.getImageWidth(),camera_.getImageHeight());   
            resize_viewport_flag_=false;
        }

        // Start imGui for this frame
        window_.newImGuiFrame(); 

        // clean last frame
        this->cleanFrame();

        // change camera according to the input
        this->moveCamera();

        // the pipeline goes well here
        this->pipelineBegin();
        
        // update frameBuffer
        window_.updateFrame(colorbuffer_->getAddr());

        
        curTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - lastTime).count();
        // std::cout<<"frame : "<<cnt++<<" ,duration:"<<duration <<" ms "<<std::endl;

        // render imGui for this frame
        window_.renderImGuiFrame();

        // postrender events
        window_.swapBuffer();

        lastTime=curTime;
        this->setDeltaTime(float(duration));


// #ifdef TIME_RECORD
//         this->timer_.report();
// #endif
    }// game loop

    window_.shutdownImGui();
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
