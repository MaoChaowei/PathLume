#include<iostream>
#include"window.h"
#include"render.h"

 // initialize static member for Window
ImGuiIO* Window::io=nullptr;                      

int main(void) {

    try{
        // define my render
        Render render;
        auto& camera=render.getCamera();

        // these setting are not open to users yet..
        camera.setMovement(0.05,0.1);
        
        // init and loop
        render.pipelineInit();
        render.GameLoop();

    }catch(const std::runtime_error& e){
        std::cout<<"error: "<<e.what()<<std::endl;
        return -1;
    }
    
    return 0;
}
