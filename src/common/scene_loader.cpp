#include"scene_loader.h"


// create BLAS for obj if it hasn't been built.
void Scene::addObjInstance_SpaceFriendly(std::string filename, glm::mat4& model,ShaderType shader,bool flipn,bool backculling){
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
    tlas_->all_instances_.emplace_back( blas_map_[filename],model,shader );
}

// create BLAS for obj even if it has been built before.
void Scene::addObjInstance(std::string filename, glm::mat4& model,ShaderType shader,bool flipn,bool backculling){

    // read from objfile
    ObjLoader objloader(filename,flipn,backculling);
    std::shared_ptr<ObjectDesc> obj=std::move(objloader.getObjects());
    // create blas
    std::shared_ptr<BLAS> blas=std::make_shared<BLAS>(obj,leaf_num_);

    objloader.updateNums(vertex_num_,face_num_);
    std::cout<<"Current vertex num: "<<vertex_num_<<std::endl;
    std::cout<<"Current face num: "<<face_num_<<std::endl;
    
    // create ASInstance for obj
    tlas_->all_instances_.emplace_back( blas,model,shader );
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
        auto object=inst.blas_->object_;
        inst.blas_=std::make_shared<BLAS>(object,leaf_num_);
    }
}