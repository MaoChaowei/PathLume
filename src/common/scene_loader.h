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
     * @brief Before ray tracing, Scene object must have used this function to collect all the emissive faces
     * 
     */
    void findAllEmitters(){
        emits_.clear();
        
        for(auto& inst:tlas_->all_instances_){
            auto& obj=inst->blas_->object_;
            auto& facenormal=obj->getFaceNorms();

            for(int face=0;face<obj->getFaceNum();++face){
                auto mtl=obj->getFaceMtl(face);
                if(mtl&&mtl->isEmissive()){
                    emits_.addEmitter(&obj->getOneVertex(face,0),&obj->getOneVertex(face,1),&obj->getOneVertex(face,2),
                                        inst->modle_*glm::vec4(facenormal[face],0),
                                        mtl->radiance_rgb_);
                }
            }
        }
        emits_.setPreSum();
    }
    void sampleEmitters(const glm::vec3& src_pos,LightSampleRecord& lsRec,Sampler& sampler)const{

        float u0=sampler.getSample1D();         // sample a triangle
        glm::vec2 u12=sampler.getSample2D();    // sample a point inside the triangle
        emits_.sampleLight(src_pos,lsRec,u0,u12.x,u12.y);

        lsRec.pdf_=std::clamp(lsRec.pdf_,0.00001f,10000.f);

        return;
    }

    float getLightPDF(const Ray& ray,const IntersectRecord& inst)const{
        float pdf=emits_.getSamplePDF(ray,inst);

        // For numerical stability
        pdf=std::clamp(pdf,1e-3f,srender::MAXPDFVALUE);

        return pdf;
    }

    

    /**
     * @brief Evaluate Scene Scale by the top tlas node's bvh box
     * 
     * @return greatest length of this box
     */
    float getSceneScale()const {
        auto maxbox=tlas_->tree_->at(0).bbox;
        float maxsize=0;
        for(int i=0;i<3;++i)
            maxsize=std::max(maxsize,maxbox.length(i));
        return maxsize;
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

    // Take charge of ALL the tangible lights in the scene
    Emitters emits_;
    // std::vector<Emitters> all_emits_;
    // std::vector<float> emits_psum_; //  the pre-sum of emitter's weight (emits_psum[i+1] is the sum of all_emits_[0~i].total_weight
    // float total_weight_;

};





/**
     * @brief Before ray tracing, Scene object must have used this function to collect all the emissive faces
     * 
     */
    // void findAllEmitters(){

    //     std::unordered_map<std::string,int> emits_map_;
    //     all_emits_.clear();
    //     emits_psum_.clear();

    //     for(auto& inst:tlas_->all_instances_){
    //         auto& obj=inst->blas_->object_;
    //         auto& facenormal=obj->getFaceNorms();

    //         for(int face=0;face<obj->getFaceNum();++face){
    //             auto mtl=obj->getFaceMtl(face);
    //             if(mtl&&mtl->isEmissive()){
    //                 std::string emit_name=mtl->getName();
    //                 Emitters* emit;
    //                 if(emits_map_.find(emit_name)!=emits_map_.end()){
    //                     emit=&all_emits_[emits_map_[emit_name]];
    //                 }else{
    //                     uint32_t idx=emits_map_.size();
    //                     emits_map_[emit_name]=idx;
    //                     all_emits_.emplace_back();
    //                     assert(all_emits_.size()==idx+1);

    //                     emit=&all_emits_.back();
    //                 }

    //                     emit->addEmitter(&obj->getOneVertex(face,0),&obj->getOneVertex(face,1),&obj->getOneVertex(face,2),
    //                                     inst->modle_*glm::vec4(facenormal[face],0),
    //                                     mtl->radiance_rgb_);
    //             }
    //         }
    //     }

    //     // prepare the pre-sum of emitter's weight
    //     if(all_emits_.size()){
    //         std::vector<float> temp;
    //         for(auto& emit:all_emits_){
    //             emit.setPreSum();
    //             float delta=emit.getWeight();
    //             temp.emplace_back(delta);
    //             total_weight_+=delta;
    //         }

    //         emits_psum_.resize(temp.size()+1);
    //         emits_psum_[0]=0;
    //         for(int i=0;i<temp.size();++i){
    //             emits_psum_[i+1]=emits_psum_[i]+temp[i]/total_weight_;
    //         }

    //         assert(fabs(emits_psum_.back()-1.0)<srender::EPSILON);
    //     }
    // }

    // void sampleEmitters(const glm::vec3& src_pos,LightSampleRecord& lsRec,Sampler& sampler)const{
    //     if(all_emits_.size()<=0){
    //         throw std::runtime_error("Scene::sampleEmitters: no emitter at all!");
    //     }

    //     int chosen=-1;
    //     float p_choose=0.f;

    //     // choose an emitter
    //     if(all_emits_.size()>1){
    //         float u=sampler.getSample1D();          // sample a emitter
            
    //         // Binary search for the first emitter (idx=i-1) in all_emits_ such that emits_psum_[i]>=u
    //         float totalWeight_=emits_psum_.back();
    //         int lt=0,rt=all_emits_.size()-1;

    //         while(lt<rt){
    //             int mid=(lt+rt)/2;
    //             if(emits_psum_[mid+1]<u){
    //                 lt=mid+1;
    //             }
    //             else{
    //                 rt=mid;
    //             }
    //         }
    //         chosen=lt;
    //         p_choose=emits_psum_[lt+1]-emits_psum_[lt];
    //         assert(all_emits_.size()>lt);

    //     }else{
    //         chosen=0;
    //         p_choose=1.0f;
    //     }

    //     // sample the emitter
    //     float u0=sampler.getSample1D();         // sample a triangle
    //     glm::vec2 u12=sampler.getSample2D();    // sample a point inside the triangle

    //     all_emits_[chosen].sampleLight(src_pos,lsRec,u0,u12.x,u12.y);
    //     lsRec.pdf_*=p_choose;                   // add the prob. of choosing the emitter

    //     // if the pdf is an outlier , ignore this shadow ray
    //     if(lsRec.pdf_<srender::EPSILON||lsRec.pdf_>srender::MAXPDFVALUE) 
    //         lsRec.shadow_ray_=nullptr;

    //     return;
    // }

    // /**
    //  * @brief Get PDF(wi) of a given sampled ray which hits on a given point.
    //  *        Note that the field of intergrand(wi) covers all the light
    //  */
    // float getLightPDF(const Ray& ray,const IntersectRecord& inst)const{
    //     if(all_emits_.empty() || glm::dot(ray.dir_,inst.normal_)>0.f)
    //         throw std::runtime_error("Scene::getLightPDF: Invalid case!");

    //     float costheta=std::max(glm::dot(inst.normal_,glm::normalize(-ray.dir_)),0.f);    // ignore the back face

    //     if(inst.t_<srender::EPSILON)
    //         return 0.f;

    //     float squred_dist=inst.t_*inst.t_;

    //     float G=costheta/squred_dist;
    //     float pdf= 1.0*(utils::getLuminance(inst.material_->radiance_rgb_))/(total_weight_*G);

    //     // For numerical stability
    //     pdf=std::clamp(pdf,1e-3f,srender::MAXPDFVALUE);

    //     return pdf;
    // }