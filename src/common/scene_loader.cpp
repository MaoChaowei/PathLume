#include"scene_loader.h"


// create BLAS for obj if it hasn't been built.
void Scene::addObjInstance(std::string filename, glm::mat4& model,ShaderType shader,bool flipn,bool backculling){
    if(blas_map_.find(filename)==blas_map_.end()){
        // read from objfile
        ObjLoader objloader(filename,flipn,backculling);
        std::shared_ptr<ObjectDesc> obj=std::move(objloader.getObjects());
        // create blas
        blas_map_[filename]=std::make_shared<BLAS>(obj,leaf_num_);

        objloader.updateNums(vertex_num_,face_num_);
        std::cout<<"Current vertex num: "<<vertex_num_<<std::endl;
        std::cout<<"Current face num: "<<face_num_<<std::endl;
    }
    // create ASInstance for obj
    tlas_->all_instances_.emplace_back( std::make_shared<ASInstance>(blas_map_[filename],model,shader) );
}


void Scene::clearScene(){
    tlas_=std::make_unique<TLAS>();
    all_lights_.clear();
    blas_map_.clear();
    vertex_num_=0;
    face_num_=0;
}

// when leaf_num is changed, blas should be rebuilt.
void Scene::rebuildBLAS(){
    for(auto& inst:tlas_->all_instances_){
        auto object=inst->blas_->object_;
        inst->blas_=std::make_shared<BLAS>(object,leaf_num_);
    }
}


/**
 * @brief Before ray tracing, Scene object must have used this function to collect all the emissive faces
 * 
 */
void Scene::findAllEmitters(){
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
void Scene::sampleEmitters(const glm::vec3& src_pos,LightSampleRecord& lsRec,Sampler& sampler)const{

    float u0=sampler.getSample1D();         // sample a triangle
    glm::vec2 u12=sampler.getSample2D();    // sample a point inside the triangle
    emits_.sampleLight(src_pos,lsRec,u0,u12.x,u12.y);

    lsRec.pdf_=std::clamp(lsRec.pdf_,0.00001f,10000.f);

    return;
}

float Scene::getLightPDF(const Ray& ray,const IntersectRecord& inst)const{
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
float Scene::getSceneScale()const {
    auto maxbox=tlas_->tree_->at(0).bbox;
    float maxsize=0;
    for(int i=0;i<3;++i)
        maxsize=std::max(maxsize,maxbox.length(i));
    return maxsize;
}