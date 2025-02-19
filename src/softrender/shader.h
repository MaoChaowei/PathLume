#pragma once
#include"common/common_include.h"
#include"common/AABB.h"
#include"common/cputimer.h"
#include"common/enumtypes.h"
#include"vertex.h"
#include"object.h"
#include"material.h"
#include"light.h"
#include"camera.h"



// per-primitive design,make sure each fragment knows its' context in the pipeline
struct ShaderContentRecord{

    // vertex shader output
    Vertex* v[3];             // all attribute of vetices
    PrimitiveType primitive_type; 

    //---after interpolation----

    // fragment shader input
    float depth;            // ndc depth
    glm::vec3 normal;       // world normal
    glm::vec3 vbary;        // barycenter in view space
    glm::vec3 fragpos;      // world position
    glm::vec2 uv;
    glm::vec4 in_color;
    

    // fragment shader output
    glm::vec4 color;
    
    ShaderContentRecord(){}

    // For implementaion of scan-line conversion
    // no need to interpolate by barycenter coordinate, instead using incremental result directly.
    void bindFragment(const FragmentHolder& holder){
        depth=holder.depth_;
        normal=holder.w_norm_;
        fragpos=holder.w_pos_;
        uv=holder.uv_;
        in_color=holder.color_;
    }
};

class Shader{
public:
    Shader(){}
    
    inline void bindMVP(const glm::mat4* m){ mvp_=m;}
    inline void bindViewport(const glm::mat4* m){ viewport_=m;}
    inline void bindNormalMat(const glm::mat4* m){ normal_mat_=m;}
    inline void bindModelMat(const glm::mat4* m){ model_mat_=m;}
    inline void bindMaterial(std::shared_ptr<Material> mp){ material_=mp; }
    inline void bindCamera(std::shared_ptr<Camera> cp){ camera_=cp; }
    inline void bindLights(const std::vector<std::shared_ptr<Light>>& light){ lights_=light; }
    inline void bindTimer(CPUTimer* t){ timer_=t; }
    inline void bindFragmentHolder(const FragmentHolder& f){content_.bindFragment(f);}

    void setShaderType(ShaderType st);

    inline void setPrimitiveType( PrimitiveType t){ content_.primitive_type=t; }

    inline void setFrustum(int near,int far){
        near_plane_=near;
        far_plane_=far;
    }
    
    inline ShaderType getType()const{ return type_;}
    inline glm::vec4 getColor()const { return content_.color; }
    inline float getDepth() const { return content_.depth; }
    inline glm::vec3 getScreenPos(uint32_t idx) const { return content_.v[idx]->s_pos_; }
    inline Vertex* getVertices(uint32_t idx) { return content_.v[idx]; }

    inline bool checkShader(ShaderType flag){
        return (bool)(flag&type_);
    }
    inline bool checkInterpSign(InterpolateSignal flag){
        return (bool)(flag&interp_sign_);
    }

    inline void assemblePrimitive(Vertex* v1, Vertex* v2, Vertex* v3){
        content_.v[0]=v1;
        content_.v[1]=v2;
        content_.v[2]=v3;
    }

    /**
     * @brief vertex shader: convert vertex from model-space to screen-space 
     *        => x and y in [width,height], z in [-1,1];
     *        convert normals into world space
     * @param v : the vertex information holder
     */
    void vertexShader(Vertex& v );
    void vertex2Screen(Vertex& v );
    void vertex2Screen(Vertex& v,AABB3d& box,AABB3d& screen_box);

    /**
     * @brief shading the fragment with properties set in `ShaderType`.
     * 
     * @param xy : refer to the position in screen-space
     * @param cur_depth : refer to the current z-buffer(x,y) so that I can finish Early-Z Test here.
     * @return if the fragment can be accepted for now, than return `true`, ortherwise return `false`.
     */
    void fragmentShader(uint32_t x,uint32_t y);
    void fragmentInterp(uint32_t x,uint32_t y);
    float fragmentDepth(uint32_t x,uint32_t y);
    void fragmentShader(FragmentHolder& fragment );

    // different shaders
    void blinnphoneShader(Material& mtl);
    void textureShader();
    void colorShader();
    void depthShader();
    void normalShader();
    void lightShader();

    // local light calculation 
    void shadeDirectLight(const DirLight& light,const glm::vec3& normal,const glm::vec3& eyepos,const glm::vec3& fpos,const Material& mtl);
    void shadePointLight(const PointLight& light,const glm::vec3& normal,const glm::vec3& eyepos,const glm::vec3& fpos,const Material& mtl);


private:
    ShaderContentRecord content_;
    ShaderType type_;
    InterpolateSignal interp_sign_;

    const glm::mat4 * mvp_;
    const glm::mat4 * viewport_;     // mvpv
    const glm::mat4 * normal_mat_;    // m-1T
    const glm::mat4 * model_mat_;     // m
    std::shared_ptr<Material> material_;
    std::vector<std::shared_ptr<Light>> lights_;
    std::shared_ptr<Camera> camera_;

    float far_plane_;
    float near_plane_;

    CPUTimer* timer_;

};





