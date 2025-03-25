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

    glm::vec3 Li(const Ray ray,PathTraceRecord& pRecord) override{

        const Scene& scene=pRecord.scene;
        Sampler& sampler=pRecord.sampler;

        // For turning recursion into loop, `throughput` multiplies bsdf*cos/pdf each bounce.
        glm::vec3 throughput(1.0);
        // record the radiance along the path
        glm::vec3 radiance(0.f);
        // record the current ray
        Ray curRay(ray);
        // Trace the current ray
        std::shared_ptr<IntersectRecord> inst=traceRay(curRay,&scene);
        if(!inst){
            radiance+=glm::vec3(0.f);// could be an environment map 
            return radiance;
        }

        // Start Path Tracing!
        while((pRecord.curdepth++)<max_depth_||max_depth_<=0){

            /*-----------------------  Emission ------------------------*/
            auto& mtl=inst->material_;
            if(!mtl){
                throw std::runtime_error("Li(const Ray ray,PathTraceRecord& pRecord): the hit point doesn't own a material!");
            }
            // Attention:  Only consider self-emssion for the first intersection, 
            // because the following bounces are considered only in the term of "Indirect Light"
            if((int)(mtl->type_&MtlType::Emissive)&&pRecord.curdepth==1){
                radiance+=throughput*mtl->getEmit();
            }


            auto bsdf=inst->getBSDF();
            bool pure_specular=(bool)(mtl->type_==MtlType::Specular);

            /*-----------------------Sample Direct Light------------------------*/
            // Sampling a Direct Light
            glm::vec3 direct(0.f);
            int t=pRecord.light_split;
            while(!pure_specular&&t--){
                LightSampleRecord lsRec;
                glm::vec3 adjust_pos=inst->pos_+inst->normal_*(float)(0.001);    //prevent from self-intersection
                scene.sampleEmitters(adjust_pos,lsRec,sampler);  

                // Trace a Shadow Ray
                if(lsRec.shadow_ray_){ 

                    std::shared_ptr<IntersectRecord> light_inst=traceRay(*lsRec.shadow_ray_,&scene);
                    // if visible, update radiance
                    if(light_inst&&fabs(light_inst->t_-lsRec.dist_)<lsRec.dist_*0.01){
                        
                        glm::vec3 wo=inst->ray2TangentSpace(-curRay.dir_);
                        glm::vec3 wi=inst->ray2TangentSpace(lsRec.shadow_ray_->dir_);

                        BSDFRecord bsdfRec(*inst,sampler,wo,wi);
                        bsdf->evalBSDF(bsdfRec);
                        float cosTheta = std::max(0.f, wi.z);

                        // calculate MIS weight
                        float weight=getMISweight(lsRec.pdf_,bsdfRec.pdf);
                        if(isnan(weight)){
                            throw std::runtime_error("isnan(weight)");
                        }

                        direct+=throughput*bsdfRec.bsdf_val*lsRec.value_*cosTheta*weight;
                    }
                }
            }
            radiance+=direct/float(pRecord.light_split);
            

             /*-----------------------Sample BSDF------------------------*/

            BSDFRecord bsdfRec(*inst,sampler,-curRay.dir_);
            bsdf->sampleBSDF(bsdfRec);
            bsdf->evalBSDF(bsdfRec);
            if(!bsdfRec.isValid())// If bsdf value or pdf is too small, this path would gain us little benefit. 
                break;
            
            // generate next direction and trace it
            glm::vec3 wi_world=inst->wi2WorldSpace(bsdfRec.wi);
            curRay=Ray(inst->pos_+inst->normal_*0.001f,wi_world);
            inst=traceRay(curRay,&scene);
            if(!inst){
                break;
            }

            throughput*=bsdfRec.bsdf_val*bsdfRec.costheta/bsdfRec.pdf;

            //  if meet an emitter , calculate MIS weight and terminate
             if((int)(inst->material_->type_&MtlType::Emissive)){
                auto Li=inst->material_->radiance_rgb_;
                float light_prob=scene.getLightPDF(curRay,*inst);
                float weight= pure_specular?1.0:getMISweight(bsdfRec.pdf,light_prob);
                if(isnan(weight)){
                    throw std::runtime_error("isnan(weight)");
                }
                radiance+=throughput*Li*weight;
                break;
             }



            if(max_depth_<=0||pRecord.curdepth<max_depth_){
                /*-----------------------InDirect Light------------------------*/
                /* Russian Roulette */
                float RR=std::max(std::min((throughput[0]+throughput[1]+throughput[2])*0.3333333f,0.75f),0.2f);
                if(pRecord.sampler.pcgRNG_.nextFloat()<RR){
                    throughput/=RR;
                }
                else
                    break;
            }

        }
        
        return radiance;
    }

private:
    /**
     * @brief calculate MIS weight: p1^2/(p1^2+p2^2); 
     */
    float getMISweight(float p1,float p2){
        auto p1_s=p1*p1;
        return (p1_s)/(p1_s+p2*p2);
    }
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

    glm::vec3 Li(const Ray ray,PathTraceRecord& pRecord) override{

        const Scene& scene=pRecord.scene;
        Sampler& sampler=pRecord.sampler;

        // For turning recursion into loop, `throughput` multiplies bsdf*cos/pdf each bounce.
        glm::vec3 throughput(1.0);
        // record the radiance along the path
        glm::vec3 radiance(0.f);
        // record the current ray
        Ray curRay(ray);
        // Trace the current ray
        std::shared_ptr<IntersectRecord> inst=traceRay(curRay,&scene);
        if(!inst){
            radiance+=glm::vec3(0.f);//I can possibly use an environment map.
            return radiance;
        }

        // Start Path Tracing!
        while((pRecord.curdepth++)<max_depth_||max_depth_<=0){

            /*-----------------------  Emission ------------------------*/
            auto& mtl=inst->material_;
            if(!mtl){
                throw std::runtime_error("Li(const Ray ray,PathTraceRecord& pRecord): the hit point doesn't own a material!");
            }
            // Attention:  Only consider self-emssion for the first intersection, 
            // because the following bounces are considered only in the term of "Indirect Light"
            if((int)(mtl->type_&MtlType::Emissive)&&pRecord.curdepth==1){
                radiance+=throughput*mtl->getEmit();
            }

            /*-----------------------Sample Direct Light------------------------*/

            auto bsdf=inst->getBSDF();
        
            // Sampling a Direct Light
            LightSampleRecord lsRec;
            glm::vec3 adjust_pos=inst->pos_+inst->normal_*(float)(0.001);    //prevent from self-intersection
            scene.sampleEmitters(adjust_pos,lsRec,sampler);  
            
            // Trace a Shadow Ray
            if(lsRec.shadow_ray_){ // if got a valid shadow ray, test its visibility

                std::shared_ptr<IntersectRecord> light_inst=traceRay(*lsRec.shadow_ray_,&scene);
                // if visible, update radiance
                if(light_inst&&fabs(light_inst->t_-lsRec.dist_)<lsRec.dist_*0.01){
                    
                    glm::vec3 wo=inst->ray2TangentSpace(-curRay.dir_);
                    glm::vec3 wi=inst->ray2TangentSpace(lsRec.shadow_ray_->dir_);

                    BSDFRecord bsdfRec(*inst,sampler,wo,wi);
                    bsdf->evalBSDF(bsdfRec);
                    float cosTheta = glm::dot(lsRec.shadow_ray_->dir_,inst->normal_);//std::max(0.f, wi.z);

                    // radiance+=throughput*bsdfRec.bsdf_val*lsRec.value_*cosTheta;
                    radiance+=throughput*mtl->diffuse_/srender::PI *lsRec.value_*cosTheta;
                    
                }
            }


            /*-----------------------InDirect Light------------------------*/
            if(max_depth_<=0||pRecord.curdepth<max_depth_){
                BSDFRecord bsdfRec(*inst,sampler,-curRay.dir_);
                bsdf->sampleBSDF(bsdfRec);
                bsdf->evalBSDF(bsdfRec);
                if(!bsdfRec.isValid())// If bsdf value or pdf is too small, this path would gain us little benefit. 
                    break;
                
                // generate next direction and trace it
                glm::vec3 wi_world=inst->wi2WorldSpace(bsdfRec.wi);
                curRay=Ray(inst->pos_+inst->normal_*0.001f,wi_world);
                inst=traceRay(curRay,&scene);
                if(!inst){
                    break;
                }
    
                throughput*=bsdfRec.bsdf_val*bsdfRec.costheta/bsdfRec.pdf;

                /* Russian Roulette */
                float RR=std::max(std::min((throughput[0]+throughput[1]+throughput[2])*0.3333333f,0.75f),0.2f);
                if(pRecord.sampler.pcgRNG_.nextFloat()<RR){
                    throughput/=RR;
                }
                else
                    break;
            }

        }
        
        return radiance;
    }

private:

    int max_depth_; 

};




    
// glm::vec3 Li2(const Ray ray,PathTraceRecord& pRecord) {

//     const Scene& scene=pRecord.scene;
//     Sampler& sampler=pRecord.sampler;

//     // For turning recursion into loop, `throughput` multiplies bsdf*cos/pdf each bounce.
//     glm::vec3 throughput(1.0);
//     // record the radiance along the path
//     std::vector<glm::vec3> radiances;
//     // record the current ray
//     Ray curRay(ray);

//     // Start Path Tracing!
//     while(max_depth_<=0||(pRecord.curdepth++)<max_depth_){

//         // Trace the current ray
//         std::shared_ptr<IntersectRecord> inst=traceRay(curRay,&scene);
//         if(!inst){
//             radiances.emplace_back(0);// someday, I can possibly use an environment map.
//             break;
//         }

//         /*-----------------------  Emission ------------------------*/
//         auto& mtl=inst->material_;
//         // Attention:  Only consider self-emssion for the first intersection, 
//         // because the following bounces are considered only in the case of "Indirect Light"! 
//         if(!(int)(mtl->type_&MtlType::NonEmissive)&&pRecord.curdepth==1){
//             radiances.emplace_back(throughput*mtl->getEmit());
//         }

//         /*-----------------------Direct Light------------------------*/

//         auto bsdf=inst->getBSDF(BSDFType::LambertReflection);
    
//         // Sampling Direct Light
//         LightSampleRecord lsRec;
//         glm::vec3 adjust_pos=inst->pos_+inst->normal_*(float)(0.001);    //prevent from self-intersection
//         scene.sampleEmitters(adjust_pos,lsRec,sampler);  

//         // Trace Shadow Ray
//         if(lsRec.shadow_ray_){
//             // if got a shadow ray, test its visibility
//             std::shared_ptr<IntersectRecord> light_inst=traceRay(*lsRec.shadow_ray_,&scene);
//             if(light_inst&&fabs(light_inst->t_-lsRec.dist_)<0.001){
//                 // if visible, update radiance~
//                 glm::vec3 wo=inst->ray2TangentSpace(-curRay.dir_);
//                 glm::vec3 wi=inst->ray2TangentSpace(lsRec.shadow_ray_->dir_);
//                 auto bsdf_val=bsdf->evalBSDF(wo,wi);
//                 float cosTheta = std::max(0.f, wi.z);
//                 radiances.emplace_back(throughput*bsdf_val*lsRec.value_*cosTheta);
//             }
//         }


//         /*-----------------------InDirect Light------------------------*/
//         if(max_depth_<=0||pRecord.curdepth<max_depth_){

//             BSDFRecord bsdfRec(*inst,sampler,-curRay.dir_);
//             bsdf->sampleBSDF(bsdfRec);
//             if(!bsdfRec.isValid())// If bsdf_value is too small, this path would gain us little benifit. 
//                 break;
            
//             // generate next direction
//             glm::vec3 wi_world=inst->wi2WorldSpace(bsdfRec.wi);
//             curRay=Ray(inst->pos_+inst->normal_*0.001f,wi_world);

//             throughput*=(bsdfRec.bsdf_val*bsdfRec.costheta/bsdfRec.pdf);

//             /* Russian Roulette */
//             float RR=std::max(std::min((throughput[0]+throughput[1]+throughput[2])*0.3333333f,0.95f),0.001f);
//             if(pRecord.sampler.pcgRNG_.nextFloat()<RR){
//                 throughput/=RR;
//             }
//             else
//                 break;
//         }

//     }
    
//     if(radiances.empty()){
//         return glm::vec3(0);
//     }
//     return radiances.back();

// }




    // glm::vec3 Li(const Ray ray,PathTraceRecord& pRecord)const override{

    //     const Scene& scene=pRecord.scene;
    //     Sampler& sampler=pRecord.sampler;

    //     // Trace a ray
    //     std::shared_ptr<IntersectRecord> inst=traceRay(ray,&scene);
    //     if(!inst)
    //         return glm::vec3(0.f);// background

    //     glm::vec3 radiance(0.f);
    //     glm::vec3 wo=inst->ray2TangentSpace(-ray.dir_);

    //     /*-----------------------  Emission ------------------------*/
    //     auto& mtl=inst->material_;
    //     if(!(int)(mtl->type_&MtlType::NonEmissive)){
    //         radiance+=mtl->getEmit();
    //     }

    //     /*-----------------------Direct Light------------------------*/
    //     // get bsdf
    //     auto bsdf=inst->getBSDF(BSDFType::LambertReflection);
    

    //     // Sampling Direct Light
    //     LightSampleRecord lsRec;
    //     glm::vec3 adjust_pos=inst->pos_+inst->normal_*(float)(0.001);    //prevent from self-intersection
    //     scene.sampleEmitters(adjust_pos,lsRec,sampler);  

    //     if(lsRec.shadow_ray_){
    //         // if got a shadow ray, test its visibility
    //         std::shared_ptr<IntersectRecord> light_inst=traceRay(*lsRec.shadow_ray_,&scene);
    //         if(light_inst&&fabs(light_inst->t_-lsRec.dist_)<0.001){
    //             // if visible, update radiance~
    //             glm::vec3 wi=inst->ray2TangentSpace(lsRec.shadow_ray_->dir_);
    //             auto bsdf_val=bsdf->evalBSDF(wo,wi);
    //             float cosTheta = std::max(0.f, wi.z);
    //             radiance+=bsdf_val*lsRec.value_*cosTheta;
    //         }
    //     }


    //     /*-----------------------InDirect Light------------------------*/
    //     if(max_depth_<0||(++pRecord.curdepth)<max_depth_){
    //         /* Russian Roulette */
    //         // if(pRecord.sampler.pcgRNG_.nextFloat()>0.7)// TODO: use throuput to better RR
    //         //     return radiance;

    //         BSDFRecord bsdfRec(*inst,sampler,-ray.dir_);
    //         bsdf->sampleBSDF(bsdfRec);

    //         // If bsdf_value is too small, this path would gain us little benifit. 
    //         if(!bsdfRec.isValid())
    //             return radiance;

    //         glm::vec3 wi_world=inst->wi2WorldSpace(bsdfRec.wi);
    //         Ray next(inst->pos_+inst->normal_*0.001f,wi_world);

    //         radiance+=(bsdfRec.bsdf_val*bsdfRec.costheta/bsdfRec.pdf)*Li(next,pRecord);//*(float)(1.0/0.7);

    //     }
        
    //     return radiance;
    // }
