#include"pathtracer.h"

// for specular part,use mis technique
bool needMIS(const BSDFType& bsdf_type){

    return (bool)(bsdf_type&BSDFType::BlinnPhongSpecular)||
            (bool)(bsdf_type&BSDFType::PerfectReflection);
}

glm::vec3 MonteCarloPathTracer::Li(const Ray ray,PathTraceRecord& pRecord){

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

        auto& mtl=inst->material_;
        if(!mtl){
            throw std::runtime_error("Li(const Ray ray,PathTraceRecord& pRecord): the hit point doesn't own a material!");
        }

        /*-----------------------  0.EMISSION ------------------------*/
        // for the (1)First hit with the (2)Front face of an (3)Emitter, get radiance
        bool is_emitter=(bool)(mtl->type_&MtlType::Emissive);
        if( is_emitter
            &&glm::dot(curRay.dir_,inst->normal_)<0.f
            &&pRecord.curdepth==1)
        {
            radiance+=throughput*mtl->getEmit();
        }

        //-----------------------------------------------------------//
        /*--------------------- 1.DIRECT LIGHT ----------------------*/
        //-----------------------------------------------------------//
        float u=sampler.pcgRNG_.nextFloat();
        auto bsdf=inst->getBSDF(u); // pick a bsdf among all possible bsdfs


        /* --------- MIS: Sample Light's PDF ----------*/

        glm::vec3 direct(0.f);
        int t=pRecord.light_split;

        while(!is_emitter&&t--){
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

                    bool need_mis=needMIS(bsdf->bsdf_type_);
                    float weight=need_mis?getMISweight(lsRec.pdf_,bsdfRec.pdf):1.0;

                    direct+=throughput*bsdfRec.bsdf_val*lsRec.value_*cosTheta*weight;
                }
            }
        }

        radiance+=direct/float(pRecord.light_split);
        
        /* --------- MIS: Sample BRDF's PDF ----------*/

        BSDFRecord bsdfRec(*inst,sampler,-curRay.dir_);
        bsdf->sampleBSDF(bsdfRec);
        bsdf->evalBSDF(bsdfRec);
        if(!bsdfRec.isValid())// If bsdf value or pdf is too small, this path would gain us little benefit. 
            break;
        
        // generate next direction and trace it
        glm::vec3 wi_world=inst->wi2WorldSpace(bsdfRec.wi);
        curRay=Ray(inst->pos_+inst->normal_*0.001f,wi_world);
        inst=traceRay(curRay,&scene);
        if(!inst)
            break;
        
        // update throughput (recursion)
        throughput*=bsdfRec.bsdf_val*bsdfRec.costheta/bsdfRec.pdf;


        
        bool perfect_reflect=(bool)(bsdf->bsdf_type_&BSDFType::PerfectReflection);
        bool need_mis=needMIS(bsdf->bsdf_type_);

        if((int)(inst->material_->type_&MtlType::Emissive)  // if meet an Emitter
            &&glm::dot(curRay.dir_,inst->normal_)<0.f       // front face
            &&need_mis)                                     // need mis
        {
            auto Li=inst->material_->radiance_rgb_;
            float light_prob=scene.getLightPDF(curRay,*inst);

            float weight= perfect_reflect?1.0: 
                                         getMISweight(bsdfRec.pdf,light_prob);

            radiance+=throughput*Li*weight;
            break;
        }

        //-----------------------------------------------------------//
        /*--------------------- 2.INDIRECT LIGHT --------------------*/
        //-----------------------------------------------------------//

        if(max_depth_<=0){
            /* Russian Roulette */
            float RR=std::max(std::min((throughput[0]+throughput[1]+throughput[2])*0.3333333f,0.95f),0.2f);
            if(pRecord.sampler.pcgRNG_.nextFloat()<RR){
                throughput/=RR;
            }
            else
                break;
        }

    }
    
    return radiance;
}


glm::vec3 MonteCarloPathTracerNEE::Li(const Ray ray,PathTraceRecord& pRecord) {

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
        float u=sampler.pcgRNG_.nextFloat();
        auto bsdf=inst->getBSDF(u);
    
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
                float cosTheta = std::max(0.f, wi.z);

                radiance+=throughput*bsdfRec.bsdf_val*lsRec.value_*cosTheta;
                
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