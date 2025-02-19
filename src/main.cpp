#include<iostream>
#include"window.h"
#include"render.h"

 // initialize static member for Window
ImGuiIO* Window::io=nullptr;                      

int main(void) {

    // define my render
    Render render;
    auto& camera=render.getCamera();

    // these setting are not open to users yet..
    camera.setCameraPos(glm::vec3(0.f),glm::vec3(0.f,0.f,-1.f),glm::vec3(1.f,0.f,0.f));
    camera.setFrustrum(60.0,1,1000);
    camera.setViewport(1024,1);
    render.afterCameraUpdate();
    
    camera.setMovement(0.05,0.1);
    
    // init and loop
    render.pipelineInit();
    render.GameLoop();
    
    return 0;
}
