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
    camera.setMovement(0.05,0.1);
    
    // init and loop
    render.pipelineInit();
    render.GameLoop();
    
    return 0;
}
