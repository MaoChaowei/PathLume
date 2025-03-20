/* `Window` use glfw and glad to create a basic window interface */
#pragma once
#include <GLAD/glad.h>
#include<GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include"interface.h"
#include"common_include.h"

// forward declair
class Render;

class Window{
public:
    Window(){}
    Window(const char* name,float width_factor,float height_factor){
        init(name,width_factor,height_factor);
    }

    // 初始化glfw窗口
    int init(const char* name,float width_factor,float height_factor);

    void bindRender(Render* rptr){
        render_=rptr;
    }

    void bindRenderIOInfo(RenderIOInfo* info){
        info_=info;
    }

    // 初始化着色器
    void initShaderProgram();
    // 初始化纹理和VAO
    void initData();

    inline bool shouldClose(){return glfwWindowShouldClose(window_);};
    inline void processInput(){
        return glfwPollEvents();
    };

    // 更新frameBuffer
    void updateFrame(unsigned char* addr);
    inline void swapBuffer(){glfwSwapBuffers(window_);};

    void resizeViewport(int width,int height);

    // imGui
    void initImGui();

    void newImGuiFrame();

    void ImGuiSoftRenderWindow();

    void ImGuiPathTracerWindow();

    void showProfileReport();
    
    void showFPSGraph();

    void renderImGuiFrame();

    void shutdownImGui();



private:
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void window_size_callback(GLFWwindow* window, int width, int height);

    GLFWwindow* window_;
    Render* render_;
    RenderIOInfo* info_;
    static ImGuiIO* io;
    unsigned int shaderProgram_;

    unsigned int texture_ , VAO_;
    
    float lastX_, lastY_; // 鼠标位置
    bool firstMouse_=true;     

    // 顶点着色器源代码
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            TexCoord = aTexCoord;
        }
    )";

    // 片段着色器源代码
    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        in vec2 TexCoord;
        uniform sampler2D screenTexture;
        void main() {
            FragColor = texture(screenTexture, TexCoord);
        }
    )";



};