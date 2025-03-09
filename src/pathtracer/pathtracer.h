#pragma once
#include"common/common_include.h"
#include"ray.h"
#include"scene_loader.h"
#include"hitem.h"

/**
 * @brief `PathTracer` is the base class for implemention of shading algotirhm,such as monte carlo,ambient occlusion and such.
 * 
 */
class PathTracer{
public:
    virtual ~PathTracer(){}

    glm::vec3 traceRay(const Ray& ray,const Scene* scene)const{
        assert(scene!=nullptr);
        auto& tlas=scene->getConstTLAS();

        IntersectRecord inst;
        bool hit_flag=tlas.traceRayInAccel(ray,0,inst,true);
        if(!hit_flag)
            return glm::vec3(0.0);// environment color
        
        return Li(inst);
    }

    /**
     * @brief calculate and return the color of the intersection point
     */
    virtual glm::vec3 Li(const IntersectRecord& inst)const{
        auto distance_to_color = [](float distance, float k = 0.1f) -> uint8_t {
            distance = std::max(distance, 0.0f); // 确保非负
            float scaled = 1.0f - std::exp(-k * distance); // 核心映射公式
            int pixel = static_cast<int>(255.0f * scaled + 0.5f); // 四舍五入
            pixel = std::clamp(pixel, 0, 255); // 确保范围正确
            return static_cast<uint8_t>(pixel);
        };
        // return glm::vec3(distance_to_color(inst.t_,1.0));
        glm::vec3 normal_vis = (inst.normal_ * 0.5f + 0.5f) * 255.0f;
        return normal_vis;
    }


private:

};