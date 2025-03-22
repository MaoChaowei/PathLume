#include"window.h"
#include"softrender/render.h"
#include<stdexcept>

unsigned int window_width_,window_height_;
unsigned int vp_width_ , vp_height_;
unsigned int control_width_,control_height_;

/* 一些回调函数  */ 
/**
 * @brief glfwSetWindowSize will call-back this function after `window_size_callback`
 * 
 * @param window 
 * @param width : width of window
 * @param height : height of window 
 */
void Window::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // NOTHING TO DO
} 

/**
 * @brief glfwSetWindowSize will call-back this function
 * 
 * @param window 
 * @param width : width of window
 * @param height : height of window 
 */
void Window::window_size_callback(GLFWwindow* window, int width, int height) {
    window_height_=height;
    window_width_=width;
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if(io->WantCaptureKeyboard) return; // dear imgui wants to use this inputs.

    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    Window* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (instance && instance->render_) {
        instance->render_->handleKeyboardInput(key, action);
    }

}


void Window::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if(io->WantCaptureMouse) return;    // dear imgui wants to use this inputs.

    Window* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (!instance || !instance->render_) return;

    int leftButtonState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

    if (leftButtonState == GLFW_PRESS) {
        // 如果是第一次检测到左键按下，需要重置 lastX_ 和 lastY_，避免镜头跳动
        if (instance->firstMouse_) {
            instance->lastX_ = xpos;
            instance->lastY_ = ypos;
            instance->firstMouse_ = false;
        }

        // 计算偏移量
        float xoffset = static_cast<float>(xpos - instance->lastX_);
        float yoffset = static_cast<float>(instance->lastY_ - ypos); // 注意 y 方向相反
        instance->lastX_ = static_cast<float>(xpos);
        instance->lastY_ = static_cast<float>(ypos);

        // 将偏移量传给渲染器
        instance->render_->handleMouseInput(xoffset, yoffset);
    }
    else {
        // 如果左键没有按下，可以把 firstMouse_ 重置，以便下次按下时重新设置 (可选)
        instance->firstMouse_ = true;
    }

}

// 初始化glfw窗口
int Window::init(const char* name,float width_factor,float height_factor)
{
    // Initialization
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // get primary monitor's size
     GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    if (!primaryMonitor) {
        std::cerr << "Failed to get primary monitor!" << std::endl;
        glfwTerminate();
        throw std::runtime_error("Window::init->get primary monitor's size->false... \n");
        return -1;
    }
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int screenWidth = mode->width;
    int screenHeight = mode->height;
    std::cout << "Your screen resolution : " << screenWidth << "x" << screenHeight << std::endl;

    // create a window
    window_width_=screenWidth*width_factor;
    window_height_=screenHeight*height_factor;

    // 不可缩放
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); 

	window_ = glfwCreateWindow(window_width_, window_height_, name, NULL, NULL);
	if (!window_) {
		std::cout << "fail to create the window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window_);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		glfwTerminate();
		return -1;
	}

    glfwSetWindowUserPointer(window_, this);// 设置用户指针，将 `this` 绑定到 `GLFWwindow`

    // set size for viewport
    control_height_=window_height_*0.3;
    control_width_=window_width_;

    vp_width_=window_width_;
    vp_height_=window_height_-control_height_;
	glViewport(0, window_height_-vp_height_, vp_width_, vp_height_);

    // bind call back functions
    glfwSetFramebufferSizeCallback(window_, framebuffer_size_callback);
    glfwSetWindowSizeCallback(window_, window_size_callback);
    glfwSetKeyCallback(window_, keyCallback);  
    glfwSetCursorPosCallback(window_, mouseCallback);

    this->initData();
    this->initShaderProgram();    // init shader program

    // initialize imgui.
    initImGui();

	return 0;
}

void Window::resizeViewport(int width,int height){

    vp_width_=width;
    vp_height_=height;

    // Ensure the window itself is large enough
    bool larger_window=false;
    if(vp_width_>window_width_){
        window_width_=vp_width_;
        larger_window=true;
    }
    if(vp_height_>window_height_){
        window_height_=vp_height_;
        larger_window=true;
    }
    if(larger_window){
        glfwSetWindowSize(window_,window_width_,window_height_);
    }
    
    // update Viewport here
    int vp_ltbt_=(window_height_-vp_height_)>control_height_?(window_height_-vp_height_+control_height_)*0.5:(window_height_-vp_height_);
    glViewport((window_width_-vp_width_)/2.0, vp_ltbt_, vp_width_, vp_height_);

}

// 初始化着色器
void Window::initShaderProgram() {
    // 编译顶点着色器
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // 检查编译是否成功
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // 编译片段着色器
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // 检查编译是否成功
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // 创建着色器程序并链接
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // 检查链接是否成功
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    // 删除着色器
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    shaderProgram_=shaderProgram;
}

void Window::initData()
{
    // 顶点数据和纹理坐标
    float vertices[] = {
        // positions     // texCoords
        -1.0f,  1.0f,    0.0f, 1.0f,  // 左上角
        -1.0f, -1.0f,    0.0f, 0.0f,  // 左下角
         1.0f, -1.0f,    1.0f, 0.0f,  // 右下角
         1.0f,  1.0f,    1.0f, 1.0f   // 右上角
    };
    unsigned int indices[] = {
        0, 1, 2,  // 第一三角形
        0, 2, 3   // 第二三角形
    };

    // 创建 VAO、VBO 和 EBO
    unsigned int VBO, EBO;
    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // 绑定 VAO
    glBindVertexArray(VAO_);

    // 绑定并设置 VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 绑定并设置 EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // 设置顶点属性
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);

    // 设置纹理环绕和过滤方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Window::updateFrame(unsigned char* addr){
    // if render ask to resize the viewport:
    if(render_->resize_viewport_flag_){
        resizeViewport(render_->camera_.getImageWidth(),render_->camera_.getImageHeight());   
        render_->resize_viewport_flag_=false;
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vp_width_, vp_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE,addr);// 更新纹理数据
    // clear
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    // 绘制纹理
    glUseProgram(shaderProgram_);
    glBindVertexArray(VAO_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

// imGui
void Window::initImGui(){
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = &ImGui::GetIO();

    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window_, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();
}

void Window::newImGuiFrame(){
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiSoftRenderWindow();
    ImGuiPathTracerWindow();

}


void Window::ImGuiSoftRenderWindow() {
    RasterSetting &setting = info_->raster_setting_;
    static bool first_load=true;

    if(first_load){
        ImGui::SetNextWindowPos(ImVec2(0, window_height_-control_height_)); 
        ImGui::SetNextWindowSize(ImVec2(window_width_/4.0, control_height_)); 
        first_load=false;
    }

    if (ImGui::Begin("Render Settings")) {

        ImGui::Dummy(ImVec2(0.0f, 10.0f));

        if (info_->begin_path_tracing)
            ImGui::BeginDisabled();

        // show_tlas
        ImGui::Checkbox("Show TLAS", &setting.show_tlas);

        // show_blas
        ImGui::Checkbox("Show BLAS", &setting.show_blas);

        ImGui::Text("Leaf size of BLAS");
        ImGui::SameLine();
        setting.leaf_num_change=false;
        if (ImGui::SliderInt("##Leaf size of BLAS", &setting.bvh_leaf_num, 4, 128)) {
            setting.leaf_num_change=true;
        }

        // demo_scene 
        setting.scene_change=false;
        const std::vector<std::string> demoScenes = {"hit_test","Bunny_with_wall","Bunnys_mutilights","veach-mis","cornell-box","bathroom2"};
        auto findIdx0=[&setting,&demoScenes](){
            int idx=0;
            while(idx<demoScenes.size()){
                if(demoScenes[idx]==setting.scene_filename)
                    return idx;
                ++idx;
            }
            std::cerr<<"unknown demoScenes!\n";
            return 0;
        };

        static int currentSceneIndex = findIdx0();
        ImGui::Text("    Demo Scene");
        ImGui::SameLine();
        if (ImGui::BeginCombo("##   Demo Scene", demoScenes[currentSceneIndex].c_str())) {
            for (int i = 0; i < demoScenes.size(); ++i) {
                bool isSelected = (currentSceneIndex == i);
                if (ImGui::Selectable(demoScenes[i].c_str(), isSelected)) {
                    currentSceneIndex = i;
                    if(setting.scene_filename != demoScenes[i]){
                        setting.scene_change=true;  
                        setting.scene_filename = demoScenes[i];
                    }
                }
            }
            ImGui::EndCombo();
        }

        const std::vector<std::string> rasterizeTypes = {"Naive" ,"Bvh_hzb", "Easy_hzb" ,"Scan_convert"};
        const std::vector<RasterizeType> rasterizeValues = {RasterizeType::Naive,RasterizeType::Bvh_hzb,RasterizeType::Easy_hzb,RasterizeType::Scan_convert};
        auto findIdx=[&setting,&rasterizeValues](){
            int idx=0;
            while(idx<rasterizeValues.size()){
                if(rasterizeValues[idx]==setting.rasterize_type)
                    return idx;
                ++idx;
            }
            std::cerr<<"unknown RasterizeType!\n";
            return 0;
        };

        static int currentRasterizeIndex = findIdx();
        setting.rasterize_change=false;
        ImGui::Text("Rasterize Type");
        ImGui::SameLine();
        if (ImGui::BeginCombo("##Rasterize Type", rasterizeTypes[currentRasterizeIndex].c_str())) {
            for (int i = 0; i < rasterizeTypes.size(); ++i) {
                bool isSelected = (currentRasterizeIndex == i);
                if (ImGui::Selectable(rasterizeTypes[i].c_str(), isSelected)) {
                    currentRasterizeIndex = i;
                    setting.rasterize_type=rasterizeValues[currentRasterizeIndex];
                    setting.rasterize_change=true;
                }
            }
            ImGui::EndCombo();
        }

        const std::vector<std::string> shaderTypes = {"Depth" ,"BlinnPhone", "Normal" ,"Frame"};
        const std::vector<ShaderType> shaderValues = {ShaderType::Depth ,ShaderType::BlinnPhone|ShaderType::ORDER, ShaderType::Normal ,ShaderType::Frame};
        auto findIdx2=[&setting,&shaderValues](){
            int idx=0;
            while(idx<shaderValues.size()){
                if(shaderValues[idx]==setting.shader_type)
                    return idx;
                ++idx;
            }
            std::cerr<<"unknown ShaderType!\n";
            return 0;
        };

        static int currentShaderType = findIdx2();
        setting.shader_change=false;
        ImGui::Text("   Shader Type");
        ImGui::SameLine();
        if (ImGui::BeginCombo("##   Shader Type", shaderTypes[currentShaderType].c_str())) {
            for (int i = 0; i < shaderTypes.size(); ++i) {
                bool isSelected = (currentShaderType == i);
                if (ImGui::Selectable(shaderTypes[i].c_str(), isSelected)) {
                    currentShaderType = i;
                    setting.shader_type=shaderValues[currentShaderType];
                    setting.shader_change=true;
                }
            }
            ImGui::EndCombo();
        }

        // back_culling
        ImGui::Checkbox("Backface Culling", &setting.back_culling);

        // profile_report
        ImGui::Checkbox("Profile Report", &info_->profile_report);

        if (info_->profile_report) {
            showProfileReport();
        }else{
            ImGui::Text("Enable 'Profile Report' to view metrics.");
        }

        

        if (info_->begin_path_tracing)
            ImGui::EndDisabled();

    
    }
    ImGui::End();

}

void Window::ImGuiPathTracerWindow(){
    static bool first_load=true;
    if(first_load){
        ImGui::SetNextWindowPos(ImVec2(window_width_/4.0, window_height_-control_height_)); 
        ImGui::SetNextWindowSize(ImVec2(window_width_/4.0, control_height_)); 
        first_load=false;
    }

    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    if(ImGui::Begin("Path Tracing Setting")){
        if (info_->begin_path_tracing)
            ImGui::BeginDisabled();

        ImGui::Text("Max Depth ");
        ImGui::SameLine();
        ImGui::SliderInt("##Max Depth ", (int*)&info_->tracer_setting_.max_depth_, 0, 50);
        ImGui::Text("Tiles Size(n*n)  ");
        ImGui::SameLine();
        ImGui::SliderInt("##Tiles Size(n*n)  ", (int*)&info_->tracer_setting_.tiles_num_, 1, 16);
        ImGui::Text("Sample per Pixel ");
        ImGui::SameLine();
        ImGui::SliderInt("##Sample per Pixel ", (int*)&info_->tracer_setting_.spp_, 1,400);

        char input[256];
        strcpy(input,info_->tracer_setting_.filepath_.c_str());
        ImGui::Text("File Path ");
        ImGui::SameLine();
        if(ImGui::InputText("##File Path ", input,sizeof(input))){
            info_->tracer_setting_.filepath_=input;
            input[0]='\0';
        }
        strcpy(input,info_->tracer_setting_.filename_.c_str());
        ImGui::Text("File Name ");
        ImGui::SameLine();
        if(ImGui::InputText("##File Name ", input,sizeof(input))){
            info_->tracer_setting_.filename_=input;
            input[0]='\0';
        }

        if (info_->begin_path_tracing)
            ImGui::EndDisabled();


        ImGui::Checkbox("Begin Path tracing", &info_->begin_path_tracing);

        if(info_->begin_path_tracing){
            if(!info_->end_path_tracing)
                ImGui::BeginDisabled();
             
            bool reset=false;
            if(ImGui::Button("confirmation")){
                // TODO: export file
                std::cout<<"TODO: export file\n";
                reset=true; // reset all flags about path tracing
            }

            if (!info_->end_path_tracing)
                ImGui::EndDisabled();

            if(reset){
                info_->begin_path_tracing=false;
                info_->end_path_tracing=false;
            }
            
        }
        // TODO: file path check and warning.

    }
    ImGui::End();
}

void Window::showProfileReport(){ 
    static bool first_load=true;
    if(first_load){
        ImGui::SetNextWindowPos(ImVec2(window_width_/2.0, window_height_-control_height_)); 
        ImGui::SetNextWindowSize(ImVec2(window_width_/2.0, control_height_)); 
        first_load=false;
    }

    if (ImGui::Begin("Performance Metrics")) {

        ImGui::Columns(2, nullptr, true);// 开启 2 列布局

        PerfCnt &profile = info_->profile_;
        CPUTimer& timer=info_->rasterize_timer_;
        ImGui::Text("Image Setting:");
        ImGui::Text("Width: %d Height: %d",render_->camera_.image_width_,render_->camera_.image_height_);
        ImGui::Text("Camera Pos: (%2.f,%2.f,%2.f)",render_->camera_.position_.x,render_->camera_.position_.y,render_->camera_.position_.z);
        ImGui::Text("Camera Front: (%2.f,%2.f,%2.f)",render_->camera_.front_.x,render_->camera_.front_.y,render_->camera_.front_.z);
        ImGui::Text("Near Z: %2.f; Far Z: %2.f;",render_->camera_.near_flat_z_,render_->camera_.far_flat_z_);

        ImGui::Text("* Face(Triangle) Distribution");
        ImGui::Text("· Total Faces: %d", profile.total_face_num_);
        ImGui::Text("· Shaded Faces: %d", profile.shaded_face_num_);
        ImGui::Text("· Back Culled Faces: %d", profile.back_culled_face_num_);
        ImGui::Text("· Clipped Faces: %d", profile.clipped_face_num_);
        ImGui::Text("· HZB Culled Faces: %d", profile.hzb_culled_face_num_);

        ImGui::Dummy(ImVec2(0.0f, 10.0f)); 
        ImGui::NextColumn();
#ifdef TIME_RECORD
        ImGui::Text("* Time for Each Stage");
        for (const auto &[type, timer] : timer.timers_)
        {
            ImGui::Text("· %s : %.1f ms",type.c_str(),(timer.elapsed_time) / 1000.0);
        }
#endif
        ImGui::Columns(1);// 回到默认的单列布局
        ImGui::Separator(); 
        ImGui::Dummy(ImVec2(0.0f, 5.0f)); 

        showFPSGraph();
    }
    ImGui::End();
}

void Window::showFPSGraph() {
    
    static std::vector<float> fps(50, 0.0f); // 存储最近 100 帧的帧率
    static int currentIndex = 0;

    float currentFPS=ImGui::GetIO().Framerate;
    float currentFrameTime = 1000.0f / currentFPS;

    ImGui::Text("FPS: %.1f fps", ImGui::GetIO().Framerate);
    ImGui::Text("Frame Time: %.3f ms/frame", currentFrameTime);
    fps[currentIndex] = currentFPS;
    currentIndex = (currentIndex + 1) % fps.size();

    // 绘制帧时间折线图
    ImGui::Text("FPS Line Chart");
    ImGui::PlotLines(
        "##FPS Line Chart",             // 图表标题
        fps.data(),                     // 数据指针
        fps.size(),                     // 数据数量
        currentIndex,                    // 当前数据偏移
        nullptr,                         // 可选标签（显示在图表左上角）
        0.0f,                            // 最小值
        40.f,                           // 最大值
        ImVec2(0, 80)                    // 图表大小
    );
}

void Window::renderImGuiFrame(){
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Window::shutdownImGui(){
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
