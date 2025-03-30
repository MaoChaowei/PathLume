#include"shader.h"
#include"common/utils.h"

#define INSIDE(x,y,z,w) ((x)<(w)&&(x)>(-w))&&\
                       ((y)<(w)&&(y)>(-w))&&\
                       ((z)<(w)&&(z)>(-w))
/**
 * @brief vertex shader: convert vertex from model-space to screen-space 
 *        => x and y in [width,height], z in [-1,1];
 * @param v : the vertex information holder
 */
void Shader::vertexShader(Vertex& v ){

    v.c_pos_=(*mvp_)*glm::vec4(v.pos_,1.0f);
    v.w_pos_=(*model_mat_)*glm::vec4(v.pos_,1.0f);      
    v.w_norm_=glm::normalize( glm::vec3((*normal_mat_)*glm::vec4(v.norm_,0)));

    // if(v.c_pos_.w>1e-6 && INSIDE(v.c_pos_.x,v.c_pos_.y,v.c_pos_.z,v.c_pos_.w)){
    //     v.discard=false;  
    // }
    // else{   
    //     // need to be clipped
    //     v.discard=true;
    // }
}

void Shader::vertex2Screen(Vertex& v ){
    // perspective division and viewport transformation
    assert(v.c_pos_.w>0.f);
    v.s_pos_=(*viewport_)*(v.c_pos_/v.c_pos_.w);     
}

// use this function to maintain a aabb3d box in screenspace
void Shader::vertex2Screen(Vertex& v,AABB3d& box,AABB3d& screen_box){
    // perspective division and viewport transformation
    v.s_pos_=(*viewport_)*(v.c_pos_/v.c_pos_.w);     

    // ensure all the vertices went through this process is inside the screen.
    v.s_pos_.x=std::clamp(v.s_pos_.x,screen_box.min.x,screen_box.max.x);
    v.s_pos_.y=std::clamp(v.s_pos_.y,screen_box.min.y,screen_box.max.y);

    box.addPoint(v.s_pos_);

}

float Shader::fragmentDepth(uint32_t x,uint32_t y){
    auto& v1=content_.v[0];
    auto& v2=content_.v[1];
    auto& v3=content_.v[2];

    glm::vec3 bary=utils::getBaryCenter(v1->s_pos_,v2->s_pos_,v3->s_pos_,glm::vec2(x,y));
    glm::vec3 correct_bary;

    // Check whether [x,y] sit inside the triangle. 
    // HOWEVER, I found a loose epsilon here is necessary, otherwise, some boundary pixels will be thrown
    // which results in notable noises at the edge.
    const float eps=-0.001;
    if(bary.x<eps||bary.y<eps||bary.z<eps)
        return srender::FAR_Z*2;   // farther than far plane
    
    // perspective correct interpolation
    for(int i=0;i<3;++i)
        correct_bary[i]=bary[i]/content_.v[i]->c_pos_.w;

    float z_n=1.0/(correct_bary[0]+correct_bary[1]+correct_bary[2]);

    for(int i=0;i<3;++i)
        correct_bary[i]*=z_n;
    
    content_.vbary=correct_bary;
    
    // depth-interpolation
    content_.depth = v1->s_pos_.z * correct_bary[0] + v2->s_pos_.z * correct_bary[1] + v3->s_pos_.z * correct_bary[2];

    return content_.depth;
}

void Shader::fragmentInterp(uint32_t x,uint32_t y){
    auto& v1=content_.v[0];
    auto& v2=content_.v[1];
    auto& v3=content_.v[2];

    // normal-interpolation.
    if(checkInterpSign(InterpolateSignal::Normal)){
        for(int i=0;i<3;++i)
            content_.normal[i] = v1->w_norm_[i] * content_.vbary[0] + v2->w_norm_[i] * content_.vbary[1] + v3->w_norm_[i] * content_.vbary[2];
        content_.normal=glm::normalize(content_.normal);
    }

    // position in world space
    if(checkInterpSign(InterpolateSignal::FragPos_World)){
        for(int i=0;i<3;++i)
            content_.fragpos[i]=glm::dot(glm::vec3(v1->w_pos_[i],v2->w_pos_[i],v3->w_pos_[i]),content_.vbary);
    }

    // uv
    if(checkInterpSign(InterpolateSignal::UV)){
        content_.uv[0]=glm::dot(glm::vec3(v1->uv_[0],v2->uv_[0],v3->uv_[0]),content_.vbary);
        content_.uv[1]=glm::dot(glm::vec3(v1->uv_[1],v2->uv_[1],v3->uv_[1]),content_.vbary);
    }

    // color
    if(checkInterpSign(InterpolateSignal::Color)){
        for(int i=0;i<4;++i)
            content_.in_color[i] =glm::dot(glm::vec3(v1->color_[i],v2->color_[i],v3->color_[i]),content_.vbary);
    }

}

void Shader::fragmentShader(uint32_t x,uint32_t y){

    assert(content_.primitive_type==PrimitiveType::MESH);
    /*--------------------- Interpolate ---------------------*/

    fragmentInterp(x,y);

    /*---------------------   Shading   ---------------------*/

    if(checkShader(ShaderType::Texture)&&material_)                               
        textureShader();

    else if(checkShader(ShaderType::Color))
        colorShader();
    
    else if(checkShader(ShaderType::Depth))
        depthShader();
    
    else if(checkShader(ShaderType::Normal))
        normalShader();
    
    else if(checkShader(ShaderType::Light))
        lightShader();

    content_.color=glm::clamp(content_.color,glm::vec4(0),glm::vec4(255.0));
}

void Shader::fragmentShader(FragmentHolder& fragment ){
    bindFragmentHolder(fragment);
    if  (checkShader(ShaderType::Texture)&&material_)                               
        textureShader();

    else if(checkShader(ShaderType::Color))
        colorShader();
    
    else if(checkShader(ShaderType::Depth))
        depthShader();
    
    else if(checkShader(ShaderType::Normal))
        normalShader();
    
    else if(checkShader(ShaderType::Light))
        lightShader();

    // color processing...
    for(int i=0;i<3;++i){
        content_.color[i]=std::min(255.f,content_.color[i]);
    }
}

void Shader::blinnphoneShader(Material& mtl){
    content_.color=glm::vec4(0.f);

    auto& v1=content_.v[0];
    auto& v2=content_.v[1];
    auto& v3=content_.v[2];

    // multiple lights shading by blinn-phong shader
    for(auto light:lights_){
        if(LightType::Dirction==light->type_){
            std::shared_ptr<DirLight> ptr=std::dynamic_pointer_cast<DirLight>(light);
            if(ptr){
                shadeDirectLight(*ptr,content_.normal,camera_->getPosition(),content_.fragpos,mtl); 
            }else{
                std::cerr<<"fail to get ptr!\n";
                exit(-1);
            }
        }else{
            assert(LightType::Point==light->type_);
            std::shared_ptr<PointLight> ptr=std::dynamic_pointer_cast<PointLight>(light);
            if(ptr){
                shadePointLight(*ptr,content_.normal,camera_->getPosition(),content_.fragpos,mtl);
            }else{
                std::cerr<<"fail to get ptr!\n";
                exit(-1);
            }
        }
    }
}

void Shader::textureShader(){
    Material temp;  // record materials of this fragment
    auto& v1=content_.v[0];
    auto& v2=content_.v[1];
    auto& v3=content_.v[2];

    auto ami=material_->getTexture(MltMember::Ambient);
    auto diff=material_->getTexture(MltMember::Diffuse);
    auto spec=material_->getTexture(MltMember::Specular);

    auto& u=content_.uv[0];
    auto& v=content_.uv[1];
    if(diff){// get diffuse color
        temp.diffuse_= diff->getColorBilinear(u,v);
    }else{
        temp.diffuse_= (255.0f)*material_->diffuse_;
    }

    if(!checkShader(ShaderType::LIGHTSHADER)){
        content_.color=glm::vec4(temp.diffuse_,1.0f);
    }
    else{
        if(ami){// get ambient color
            temp.ambient_= ami->getColorBilinear(u,v);
        }else{
            temp.ambient_= (255.0f)*material_->ambient_;
        }
        if(spec){// get specular color
            temp.specular_= spec->getColorBilinear(u,v);
        }else{
            temp.specular_= (255.0f)*material_->specular_;
        }
        if(checkShader(ShaderType::BlinnPhone))
            blinnphoneShader(temp);
    }
}

void Shader::colorShader(){
    auto& v1=content_.v[0];
    auto& v2=content_.v[1];
    auto& v3=content_.v[2];

    if(!checkShader(ShaderType::LIGHTSHADER)){
        content_.color=content_.in_color;
    }
    else{
        Material temp;
        temp.diffuse_=content_.in_color;
        temp.ambient_=temp.diffuse_;
        temp.specular_=(0.2f)*temp.diffuse_;

        if(checkShader(ShaderType::BlinnPhone))
            blinnphoneShader(temp);
    }
}

void Shader::lightShader(){
    content_.color=glm::vec4(255,255,255,1);
}

void Shader::depthShader(){
    float dist=(2.0*near_plane_*far_plane_)/(-content_.depth*(far_plane_-near_plane_)+(far_plane_+near_plane_));        // from ndc Zn to view space -Ze(since Zn is non-linear!)
    content_.color=glm::vec4( (255.0*(dist-far_plane_))/(near_plane_-far_plane_));
}

void Shader::normalShader(){
    content_.color= glm::vec4((content_.normal * 0.5f + 0.5f)*255.0f, 1.0); 
}


void Shader::shadePointLight(const PointLight& light,const glm::vec3& normal,const glm::vec3& eyepos,const glm::vec3& fpos,const Material& mtl){
    // glm::vec3 n_normal=glm::normalize(normal);  
    glm::vec3 toLight=glm::vec3(light.pos_-fpos);
    float dist_squred=glm::dot(toLight,toLight);
    toLight=glm::normalize(toLight);
    glm::vec3 toEye=glm::normalize(glm::vec3(eyepos-fpos));

    glm::vec3 halfh=glm::normalize(toLight+toEye);
    float cosa=glm::dot(toLight,normal);

    float speccoef=std::pow(std::max(0.f,glm::dot(halfh,normal)),mtl.getShininess());

    glm::vec3 ambientcolor=light.ambient_*mtl.getAmbient();
    glm::vec3 diffcolor=light.diffuse_*mtl.getDiffuse()*std::max(0.f,cosa);
    glm::vec3 specolor=light.specular_*mtl.getSpecular()*speccoef;

    float dist_attenuation=light.quadratic_*dist_squred;
    float attenuation=dist_attenuation>1?1.0/dist_attenuation:1.0;

    content_.color+=glm::vec4((ambientcolor+diffcolor+specolor)*attenuation,1.0f);
}

void Shader::shadeDirectLight(const DirLight& light,const glm::vec3& normal,const glm::vec3& eyepos,const glm::vec3& fpos,const Material& mtl){
    // glm::vec3 n_normal=glm::normalize(normal);  
    glm::vec3 toLight=glm::normalize(-light.dir_);
    glm::vec3 toEye=glm::normalize(glm::vec3(eyepos-fpos));

    glm::vec3 halfh=glm::normalize(toLight+toEye);
    float cosa=glm::dot(toLight,normal);
    float speccoef=std::pow(std::max(0.f,glm::dot(halfh,normal)),mtl.getShininess());

    glm::vec3 ambientcolor=light.ambient_*mtl.getAmbient();
    glm::vec3 diffcolor=light.diffuse_*mtl.getDiffuse()*std::max(0.f,cosa);
    glm::vec3 specolor=light.specular_*mtl.getSpecular()*speccoef;


    content_.color+=glm::vec4((ambientcolor+diffcolor+specolor),1.0f);
}

void Shader::setShaderType(ShaderType st){
    type_=st;
    if(checkShader(ShaderType::Color|ShaderType::Light))
        interp_sign_=interp_sign_|InterpolateSignal::Color;
    if(checkShader(ShaderType::Normal|ShaderType::LIGHTSHADER))
        interp_sign_=interp_sign_|InterpolateSignal::Normal;
    if(checkShader(ShaderType::Texture|ShaderType::Light))
        interp_sign_=interp_sign_|InterpolateSignal::UV;
    if(checkShader(ShaderType::LIGHTSHADER))
        interp_sign_=interp_sign_|InterpolateSignal::FragPos_World;
}