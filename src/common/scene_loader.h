/* scene_loader defines `Scene` to control all the objects and accelerate structures */
#pragma once
#include"common_include.h"
#include"object.h"
#include"texture.h"
#include"light.h"
#include"as.h"
#include"emitter.h"
#include"sample.h"
#include<unordered_map>


class Scene{
public:
    Scene():tlas_(std::make_unique<TLAS>()){};

    void addLight(std::shared_ptr<Light> light){
        all_lights_.emplace_back(light);
    }

    void setBVHsize(uint32_t leaf_num){leaf_num_=leaf_num;}

    // create BLAS for obj if it hasn't been built.
    void addObjInstance_SpaceFriendly(std::string filename, glm::mat4& model,ShaderType shader,bool flipn=false,bool backculling=true);

    // create BLAS for obj even if it has been built before.
    void addObjInstance(std::string filename, glm::mat4& model,ShaderType shader,bool flipn=false,bool backculling=true);

    void buildTLAS(){
        tlas_->buildTLAS();
        std::cout<<"buildTLAS Done\n";
    }

    std::vector<ASInstance>& getAllInstances(){
        if(tlas_->all_instances_.size())
            return tlas_->all_instances_;
        else{
            std::cerr<<"getAllInstances() have no instaces.\n";
            exit(-1);
        }
    }

    const std::vector<std::shared_ptr<Light>>& getLights()const{
        return all_lights_;
    } 
    TLAS& getTLAS(){
        return *tlas_;
    }
    const TLAS& getConstTLAS()const{
        return *tlas_;
    }

    int getFaceNum(){return face_num_;}


    void clearScene();

    // when leaf_num is changed, blas should be rebuilt.
    void rebuildBLAS();

    /**
     * @brief Before ray tracing, Scene object must have used this function to collect all the emissive faces
     * 
     */
    void findAllEmitters(){
        emits_.clear();
        for(auto& inst:tlas_->all_instances_){
            auto& obj=inst.blas_->object_;
            auto& facenormal=obj->getFaceNorms();

            for(int face=0;face<obj->getFaceNum();++face){
                auto mtl=obj->getFaceMtl(face);
                if(mtl&&mtl->isEmissive()){
                    for(int i=0;i<3;++i){
                        emits_.addEmitter(&obj->getOneVertex(face,0),&obj->getOneVertex(face,1),&obj->getOneVertex(face,2),
                                          inst.modle_*glm::vec4(facenormal[face],0),
                                          mtl->radiance_rgb_);
                    }
                }
            }
        }
        emits_.setPreSum();
    }

    void sampleEmitters(const glm::vec3& src_pos,LightSampleRecord& lsRec,Sampler& sampler)const{

        float u0=sampler.getSample1D();         // sample a triangle
        glm::vec2 u12=sampler.getSample2D();    // sample a point inside the triangle
        emits_.sampleLight(src_pos,lsRec,u0,u12.x,u12.y);

        // if the pdf is an outlier , don't consider this shadow ray
        if(lsRec.pdf_<srender::EPSILON||lsRec.pdf_>srender::MAXPDFVALUE) 
            lsRec.shadow_ray_=nullptr;

        return;
    }

    float getLightPDF(const Ray& ray,const IntersectRecord& inst,float& G)const{
        return emits_.getSamplePDF(ray,inst,G);
    }

    
private:
    
    // AS for objects
    std::unique_ptr<TLAS> tlas_;            // TLAS->AS->BLAS->objectdesc
    std::unordered_map<std::string,std::shared_ptr<BLAS> > blas_map_;    
    int leaf_num_=4;
    int vertex_num_;
    int face_num_;

    // virtual lights
    std::vector<std::shared_ptr<Light>> all_lights_;

    // tangible lights
    Emitters emits_;
};


