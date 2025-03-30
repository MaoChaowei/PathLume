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

    /*-----------------------------------------------------------*/
    /*                   control the AS/objects                  */
    /*-----------------------------------------------------------*/
    void addLight(std::shared_ptr<Light> light){
        all_lights_.emplace_back(light);
    }

    void setBVHsize(uint32_t leaf_num){leaf_num_=leaf_num;}

    // create BLAS for obj if it hasn't been built.
    void addObjInstance(std::string filename, glm::mat4& model,ShaderType shader,bool flipn=false,bool backculling=true);

    void buildTLAS(){
        tlas_->buildTLAS();
        std::cout<<"buildTLAS Done\n";
    }

    std::vector<std::shared_ptr<ASInstance>>& getAllInstances(){
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
     * @brief Evaluate Scene Scale by the top tlas node's bvh box
     * @return greatest length of this box
     */
    float getSceneScale()const;


    /*-----------------------------------------------------------*/
    /*                     control the emitters                  */
    /*-----------------------------------------------------------*/
    /**
     * @brief Before ray tracing, Scene object must have used this function to collect all the emissive faces
     */
    void findAllEmitters();

    /**
     * @brief For direct light sampling
     */
    void sampleEmitters(const glm::vec3& src_pos,LightSampleRecord& lsRec,Sampler& sampler)const;

    /**
     * @brief For calculation of MIS weight 
     */
    float getLightPDF(const Ray& ray,const IntersectRecord& inst)const;

    
private:
    
    // AS for objects
    std::unique_ptr<TLAS> tlas_;            // TLAS->AS->BLAS->objectdesc
    std::unordered_map<std::string,std::shared_ptr<BLAS> > blas_map_;    
    int leaf_num_=4;
    int vertex_num_;
    int face_num_;

    // virtual lights
    std::vector<std::shared_ptr<Light>> all_lights_;

    // Take charge of ALL the tangible lights in the scene
    Emitters emits_;

};


