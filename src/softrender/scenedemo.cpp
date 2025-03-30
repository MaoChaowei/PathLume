#include "render.h"
#include<unordered_map>

void Render::loadDemoScene(std::string name, ShaderType shader)
{
#ifdef TIME_RECORD
    info_.rasterize_timer_.start("loadDemoScene");
#endif
    scene_.clearScene();

    std::unordered_map<std::string,glm::vec3> lights_mtl;

    if(name=="hit_test"){
        
        glm::vec3 pos(278, 273, -800);
        glm::vec3 front(0,0,1);
        glm::vec3 up(0,1,0);
        setCamera(pos,pos+front, glm::cross(front,up),40,1,512,1,2000);
        {
            glm::mat4 model_matrix(1.f);
            addObjInstance(std::string("assets/model/cornellbox/floor.obj"), model_matrix, shader, false, false);
        }
        {
            glm::mat4 model_matrix(1.f);
            addObjInstance(std::string("assets/model/cornellbox/shortbox.obj"), model_matrix, shader, false, false);
        }
        {
            glm::mat4 model_matrix(1.f);
            addObjInstance(std::string("assets/model/cornellbox/tallbox.obj"), model_matrix, shader, false, false);
        }
        {
            glm::mat4 model_matrix(1.f);;
            addObjInstance(std::string("assets/model/cornellbox/left.obj"), model_matrix, shader, false, false);
        }
        {
            glm::mat4 model_matrix(1.f);
            addObjInstance(std::string("assets/model/cornellbox/right.obj"), model_matrix, shader, false, false);
        }
        {
            glm::mat4 model_matrix(1.f);
            addObjInstance(std::string("assets/model/cornellbox/light.obj"), model_matrix, shader, false, false);
        }

        lights_mtl["Light"]=8.0f * glm::vec3(0.747f+0.058f, 0.747f+0.258f, 0.747f) 
            + 15.6f * glm::vec3(0.740f+0.287f,0.740f+0.160f,0.740f) 
            + 18.4f *glm::vec3(0.737f+0.642f,0.737f+0.159f,0.737f);

       
    }
    else if (name == "Bunny_with_wall")
    {
        glm::vec3 pos(31,-85,-551);//(-309, 28, -296);
        glm::vec3 lookat(31,-85,-600);//(0, -100, -500);
        setCamera(pos, lookat, glm::cross(lookat-pos,glm::vec3(0,1,0)),60,1024/800,512,1,1000);
        {
            glm::vec3 model_position{0, -100, -400};
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), model_position);
            glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(0.f), glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)));
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(20));
            glm::mat4 model_matrix = translation * rotate * scale;
            addObjInstance(std::string("assets/model/Bunny.obj"), model_matrix, shader, false);
        }
        {
            glm::vec3 model_position{0, -100, -600};
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), model_position);
            glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(60.f), glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f))); // 60
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(120));
            glm::mat4 model_matrix = translation * rotate * scale;
            addObjInstance(std::string("assets/model/Brickwall/brickwall.obj"), model_matrix, shader, false, false);
        }
        if(0){
            glm::vec3 lightpos{100, 100, -200};
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), lightpos);
            glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(0.f), glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)));
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(2));
            glm::mat4 model_matrix = translation * rotate * scale;
            addObjInstance(std::string("assets/model/cube/cube.obj"), model_matrix, ShaderType::Light, true);

            PointLight pt;
            pt.pos_ = lightpos;
            pt.ambient_ = glm::vec3(0.1, 0.1, 0.1);
            pt.diffuse_ = glm::vec3(1, 1, 1);
            pt.specular_ = glm::vec3(0.4, 0.4, 0.4);
            pt.quadratic_ = 0.000001f;

            scene_.addLight(std::make_shared<PointLight>(pt));
        }

    }
    else if (name == "Bunnys_mutilights")
    {
        setCamera({-300, 100, 100}, {0, 0, -400}, {1, 0, 1},60,1,1024,1,1000);
        {
            glm::vec3 model_position{100, 0, -300};
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), model_position);
            glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(0.f), glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)));
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(50));
            glm::mat4 model_matrix = translation * rotate * scale;
            addObjInstance(std::string("assets/model/Bunny.obj"), model_matrix, shader, false);
        }
        {
            glm::vec3 model_position{60, 0, -500};
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), model_position);
            glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(0.f), glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)));
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(50));
            glm::mat4 model_matrix = translation * rotate * scale;
            addObjInstance(std::string("assets/model/Bunny.obj"), model_matrix, shader, false);
        }
        {
            glm::vec3 model_position{20, 0, -700};
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), model_position);
            glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(0.f), glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)));
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(50));
            glm::mat4 model_matrix = translation * rotate * scale;
            addObjInstance(std::string("assets/model/Bunny.obj"), model_matrix, shader, false);
        }
        {
            glm::vec3 lightpos{60, 150, -500};
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), lightpos);
            glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(0.f), glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)));
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(2));
            glm::mat4 model_matrix = translation * rotate * scale;
            addObjInstance(std::string("assets/model/cube/cube.obj"), model_matrix, ShaderType::Light, true);

            PointLight pt;
            pt.pos_ = lightpos;
            pt.ambient_ = glm::vec3(0.1, 0.1, 0.1);
            pt.diffuse_ = glm::vec3(1, 1, 1);
            pt.specular_ = glm::vec3(0.4, 0.4, 0.4);
            pt.quadratic_ = 0.000001f;

            scene_.addLight(std::make_shared<PointLight>(pt));
        }
        {
            glm::vec3 lightpos{0, 0, -200};
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), lightpos);
            glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(0.f), glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)));
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(2));
            glm::mat4 model_matrix = translation * rotate * scale;
            addObjInstance(std::string("assets/model/cube/cube.obj"), model_matrix, ShaderType::Light, true);

            PointLight pt;
            pt.pos_ = lightpos;
            pt.ambient_ = glm::vec3(0.1, 0.2, 0.2);
            pt.diffuse_ = glm::vec3(0, 0.5, 0.5);
            pt.specular_ = glm::vec3(0, 0.4, 0.4);
            pt.quadratic_ = 0.0001f;

            scene_.addLight(std::make_shared<PointLight>(pt));
        }
        {
            glm::vec3 lightpos{0, 0, -800};
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), lightpos);
            glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(0.f), glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)));
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(2));
            glm::mat4 model_matrix = translation * rotate * scale;
            addObjInstance(std::string("assets/model/cube/cube.obj"), model_matrix, ShaderType::Light, true);

            PointLight pt;
            pt.pos_ = lightpos;
            pt.ambient_ = glm::vec3(0.2, 0.2, 0.1);
            pt.diffuse_ = glm::vec3(1, 0.4, 0);
            pt.specular_ = glm::vec3(0.4, 0.2, 0);
            pt.quadratic_ = 0.0001f;

            scene_.addLight(std::make_shared<PointLight>(pt));
        }
    }
    else if (name == "veach-mis")
    {
        lights_mtl["light1"]=glm::vec3(300,300,300);
        lights_mtl["light2"]=glm::vec3(50,50,50);
        lights_mtl["light3"]=glm::vec3(20,20,20);
        lights_mtl["light4"]=glm::vec3(10,10,10);

        glm::vec3 eye(28.2792, 5.2, 1.23612e-06);
        glm::vec3 lookat(0, 2.8, 0);
        glm::vec3 front=lookat-eye;
        eye=glm::vec3(31,5,0);
        lookat=eye+front;
        setCamera(eye,lookat, glm::cross(lookat-eye,{0,1,0}),20.1143,1280.0/720.0,1280,1.0,100.0);
        {
            glm::mat4 model_matrix = glm::mat4(1.0f);
            addObjInstance(std::string("assets/model/veach-mis/veach-mis.obj"), model_matrix, shader, false);
        }
    }
    else if (name == "cornell-box")
    {
        lights_mtl["Light"]=glm::vec3(34.0, 24.0, 8.0);
        glm::vec3 eye(278.0, 273.0, -800);
        glm::vec3 lookat(278.0, 273.0, -799.0);
        glm::vec3 front=lookat-eye;
        // eye={287,223,-1171};
        // lookat=eye+front;
        setCamera(eye,lookat, glm::cross(front,{0,1,0}),39.3077,1024.0/1024,1024,1.0,1500.0);
        {
            glm::mat4 model_matrix = glm::mat4(1.0f);
            addObjInstance(std::string("assets/model/cornell-box/cornell-box.obj"), model_matrix, shader, false);
        }
    }
    else if (name == "bathroom2")
    {
        lights_mtl["Light"]=glm::vec3(125.0,100.0,75.0);
        glm::vec3 eye(4.443147659301758, 16.934431076049805, 49.91023254394531);
        glm::vec3 lookat(-2.5734899044036865, 9.991769790649414, -10.588199615478516);
        setCamera(eye,lookat, glm::cross(lookat-eye,{0,1,0}),35.9834,1280.0/720,1280,1.0,100.0);
        // setCamera(eye,lookat, glm::cross(lookat-eye,{0,1,0}));
        {
            glm::mat4 model_matrix = glm::mat4(1.0f);
            addObjInstance(std::string("assets/model/bathroom2/bathroom2.obj"), model_matrix, shader, false);
        }
    }
    else
    {
        std::cerr << "unknown demo~\n";
        exit(-1);
    }

    // bind light material 
    for(auto& ins:this->scene_.getAllInstances()){
        for(auto& mtl:ins->blas_->object_->getMtls()){
            if(lights_mtl.find(mtl->name_)!=lights_mtl.end()){
                // set type and rgb
                mtl->initEmissionType(MtlType::AreaLight,lights_mtl[mtl->name_]);
            }
        }
    }

#ifdef TIME_RECORD
    info_.rasterize_timer_.stop("loadDemoScene");
    // timer_.del("loadDemoScene");
#endif
}


void Render::loadXMLfile(std::string name){
    // TO IMPLEMENT
}