#include "render.h"
#include<unordered_map>

void Render::loadDemoScene(std::string name, ShaderType shader)
{
#ifdef TIME_RECORD
    info_.rasterize_timer_.start("loadDemoScene");
#endif
    scene_.clearScene();
    this->camera_.setMovement(0.05,0.1);

    std::unordered_map<std::string,glm::vec3> lights_mtl;

    if(name=="hit_test"){
        setCamera({0,0,0}, {0, 0, -1}, {1, 0, 0},60,1024/800,1024,1,500);
        {
            {
                glm::vec3 model_position{0, 100, -100};
                glm::mat4 translation = glm::translate(glm::mat4(1.0f), model_position);
                glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(60.f), glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f))); // 60
                glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(120));
                glm::mat4 model_matrix = translation * rotate * scale;
                addObjInstance(std::string("assets/model/Brickwall/brickwall.obj"), model_matrix, shader, false, false);
            }
        }
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
        /*
        <camera type="perspective" width="1280" height="720" fovy="20.1143">
            <eye x="28.2792" y="5.2" z="1.23612e-06"/> 
            <lookat x="0.0" y="2.8" z="0.0"/> 
            <up x="0.0" y="1.0" z="0.0"/> 
        </camera>
        <light mtlname="light1" radiance="300,300,300"/>
        <light mtlname="light2" radiance="50,50,50"/>
        <light mtlname="light3" radiance="20,20,20"/>
        <light mtlname="light4" radiance="10,10,10"/>
        */
        float att=0.35;
        lights_mtl["light1"]=glm::vec3(300*att,300*att,300*att);
        lights_mtl["light2"]=glm::vec3(50*att,50*att,50*att);
        lights_mtl["light3"]=glm::vec3(20*att,20*att,20*att);
        lights_mtl["light4"]=glm::vec3(10*att,10*att,10*att);

        glm::vec3 eye(28.2792, 5.2, 1.23612e-06);
        glm::vec3 lookat(0, 2.8, 0);
        // glm::vec3 front=lookat-eye;
        // eye=glm::vec3(1,7,-7);
        // lookat=eye+front;
        setCamera(eye,lookat, glm::cross(lookat-eye,{0,1,0}),20.1143,1280.0/720.0,512,1.0,100.0);
        {
            glm::mat4 model_matrix = glm::mat4(1.0f);
            addObjInstance(std::string("assets/model/veach-mis/veach-mis.obj"), model_matrix, shader, false);
        }
    }
    else if (name == "cornell-box")
    {
        /*
        <camera type="perspective" width="1024" height="1024" fovy="39.3077">
            <eye x="278.0" y="273.0" z="-800.0"/> 
            <lookat x="278.0" y="273.0" z="-799.0"/> 
            <up x="0.0" y="1.0" z="0.0"/> 
        </camera>
        <light mtlname="Light" radiance="34.0, 24.0, 8.0"/>
        */
       float att=0.35;
        lights_mtl["Light"]=glm::vec3(34.0*att, 24.0*att, 8.0*att);
        glm::vec3 eye(278.0, 273.0, -800);
        glm::vec3 lookat(278.0, 273.0, -799.0);
        glm::vec3 front=lookat-eye;
        // eye={287,223,-1171};
        // lookat=eye+front;
        setCamera(eye,lookat, glm::cross(front,{0,1,0}),39.3077,1024.0/1024,1024,1.0,2000.0);
        {
            glm::mat4 model_matrix = glm::mat4(1.0f);
            addObjInstance(std::string("assets/model/cornell-box/cornell-box.obj"), model_matrix, shader, false);
        }
    }
    else if (name == "bathroom2")
    {
        /*
        <camera type="perspective" width="1280" height="720" fovy="35.9834">
            <eye x="4.443147659301758" y="16.934431076049805" z="49.91023254394531"/> 
            <lookat x="-2.5734899044036865" y="9.991769790649414" z="-10.588199615478516"/> 
            <up x="0.0" y="1.0" z="0.0"/> 
        </camera>
        <light mtlname="Light" radiance="125.0,100.0,75.0"/>
        */
        lights_mtl["Light"]=glm::vec3(125.0,100.0,75.0);
        glm::vec3 eye(4.443147659301758, 16.934431076049805, 49.91023254394531);
        glm::vec3 lookat(-2.5734899044036865, 9.991769790649414, -10.588199615478516);
        // setCamera(eye,lookat, glm::cross(lookat-eye,{0,1,0}),35.9834,1280.0/720,1280,1.0,300.0);
        setCamera(eye,lookat, glm::cross(lookat-eye,{0,1,0}));
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