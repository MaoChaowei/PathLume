#pragma once
#include"common/common_include.h"
#include"ray.h"
#include"scene_loader.h"
#include"hitem.h"
#include"sample.h"
#include"bsdf.h"
#include<mutex>

/**
 * @brief Encapsulate all the necessary infos for tracing a light in a scene
 * 
 */
struct PathTraceRecord{
    PathTraceRecord(const Scene& sce,Sampler& sam,uint32_t lightsplit):scene(sce),sampler(sam),curdepth(0),light_split(lightsplit){}

    const Scene& scene;
    Sampler& sampler;

    int curdepth;
    int light_split;
};

/**
 * @brief `PathTracer` is the base class for implemention of shading algotirhm,such as monte carlo,ambient occlusion and such.
 * 
 */
class PathTracer{
public:
    PathTracer(){}

    virtual ~PathTracer(){}

    /**
     * @brief trace a ray into the scene, then return the intersectRecord if hitting among the desired Ray.T range ,otherwise return nullptr. 
     */
    std::shared_ptr<IntersectRecord> traceRay(const Ray& ray,const Scene* scene)const{
        assert(scene!=nullptr);
        auto& tlas=scene->getConstTLAS();

        std::shared_ptr<IntersectRecord> inst=std::make_shared<IntersectRecord>();

        bool hit_flag=tlas.traceRayInAccel(ray,0,*inst,true);

        if(!hit_flag||!ray.acceptT(inst->t_))   
            return nullptr;

        return inst;
    }

    /**
     * @brief get the radiance color of an incident ray after hitting the scene.
     */
    virtual glm::vec3 Li(const Ray ray,PathTraceRecord& pRecord){
        const Scene& scene=pRecord.scene;

        std::shared_ptr<IntersectRecord> inst=traceRay(ray,&scene);
        if(!inst)   return glm::vec3(0.f);

        glm::vec3 color = (inst->normal_ * 0.5f + 0.5f);
        
        // glm::vec3 color=inst->material_->getDiffuse(inst->uv_[0],inst->uv_[1]);

        return color;
    }


};

/**
 * @brief a simple implementation of Monte Carlo Path tracing
 * Members:
 * - max_depth_ ; <=0:infinite length,1:direct light,2: one bounce,...
 * 
 */
class MonteCarloPathTracer:public PathTracer{
public:
    MonteCarloPathTracer(int mdepth):max_depth_(mdepth){}

    glm::vec3 Li(const Ray ray,PathTraceRecord& pRecord) override;

private:
    /**
     * @brief calculate MIS weight: p1^2/(p1^2+p2^2); 
     */
    inline float getMISweight(float p1,float p2){
        auto p1_s=p1*p1;
        auto weight= (p1_s)/(p1_s+p2*p2);
        if(isnan(weight)){
            throw std::runtime_error("isnan(weight)");
        }
        return weight;
    }

    // void updateRadiance(glm::vec3& radiance,const glm::vec3& delta,float max_factor){
    //     if(utils::getLuminance(delta)>utils::getLuminance(radiance)*max_factor)
    //         radiance+=radiance*max_factor;
    //     radiance+=delta;
    // }
    int max_depth_; 
    std::mutex mx_log_;

};


/**
 * @brief a simple implementation of Monte Carlo Path tracing
 * Members:
 * - max_depth_ ; <=0:infinite length,1:direct light,2: one bounce,...
 * 
 */
class MonteCarloPathTracerNEE:public PathTracer{
public:
    MonteCarloPathTracerNEE(int mdepth):max_depth_(mdepth){}

    glm::vec3 Li(const Ray ray,PathTraceRecord& pRecord) override;

private:

    int max_depth_; 

};
