/* object.h defines all classes for object description */
#pragma once
#include"common_include.h"
#include"vertex.h"
#include "tiny_obj_loader.h"
#include"material.h"


// specify different types of objects  
enum class PrimitiveType{
    // NAME= the number of vertices
    POINT=1,
    LINE=2,
    MESH=3,
};

// forward declair
enum class ShaderType;


// the base class for all kinds of objects to be rendered
class ObjectDesc{
public:


    virtual void initObject(const tinyobj::ObjReader& reader,std::string filepath,bool flip_normals=false,bool backculling=true)=0;
    virtual void printInfo() const=0;
    virtual void clear()=0;

    std::vector<Vertex>& getVertices() { return vertices_;}
    const std::vector<Vertex>& getconstVertices() const { return vertices_;}
    std::vector<glm::vec3>& getFaceNorms() { return face_normals_;}
    const std::vector<uint32_t>& getIndices()const {return indices_;}
    const std::vector<std::shared_ptr<Material>>& getMtls()const{ return mtls_; }
    const std::vector<int>& getMtlIdx()const{ return mtlidx_; }
    const std::shared_ptr<Material> getFaceMtl(uint32_t face_idx){
        auto mtl_type_idx=mtlidx_[face_idx];
        
        if(mtl_type_idx<0) 
            return nullptr;
        else
            return mtls_[mtl_type_idx];
    }
    
    std::string getName()const{return name_;}
    PrimitiveType getPrimitiveType()const{ return type_;}
    unsigned long long getFaceNum()const{return face_num_;}

    int getVerticesNum()const{ return vertices_.size(); }

    bool isBackCulling()const{ return do_back_culling_; }
    void setBackCulling(bool flag){do_back_culling_=flag;}
    

protected:
    std::string name_;
    PrimitiveType type_;

    // all the information of vertices
    std::vector<Vertex> vertices_;  
    // contains indices to `vertices_`,and every three consecutive vertices/indices constitude a face
    std::vector<uint32_t> indices_;
    // the normal for all faces, only availabel for MESH
    std::vector<glm::vec3> face_normals_;

    // all the information of materials
    std::vector<std::shared_ptr<Material>>mtls_;  
    // each triangle's material index pointer
    std::vector<int> mtlidx_;

    // the total number of faces, hence vertices num is trible
    unsigned long long face_num_=0;  

    bool do_back_culling_=true;

    

};

class Mesh:public ObjectDesc{
public:
    Mesh(){
        type_=PrimitiveType::MESH;
    }
  
    // note the obj must be triangulated
    void initObject(const tinyobj::ObjReader& reader,std::string filepath,bool flip_normals=false,bool backculling=true)override;
    
    void clear() override{
        vertices_.clear();
    }
    
    void printInfo() const override;

public:
    bool has_normal_=true;
    bool has_uv_=true;

};

class Line:public ObjectDesc{
public:
    Line(){
        type_=PrimitiveType::LINE;
    }

    void initObject(const tinyobj::ObjReader& reader,std::string filepath,bool flip_normals=false,bool backculling=true)override{
        // to do
    }

    void clear() override{
        vertices_.clear();
    }

    // for debug use
    // void setLineDemo();
    void printInfo() const override;

private:
    // the total number of lines, hence vertices num is line_num_+1
    unsigned long long line_num_=0;   

};



// a single read from .obj results in a `ObjLoader`
class ObjLoader{
public:
    ObjLoader()=delete;

    ObjLoader(std::string str,bool flipn=false,bool backculling=true){
        // 1.set properties
        filename_=str;
        flip_normals_=flipn;
        back_culling_=backculling;
        
        // 2. do things
        readObjFile(str);
        setObject(str);
    }

    void setFileAndRead(std::string str){
        filename_=str;
        readObjFile(str);
    }

    inline std::string getFilename()const{
        return filename_;
    }

    inline const tinyobj::ObjReader& getReader() const{
        return reader_; // read only
    }

    std::unique_ptr<ObjectDesc>& getObjects(){
        return object_;
    }

    std::vector<std::unique_ptr<Material>>& getMtls(){
        return all_mlts_;
    }

    inline void updateNums(int& vn,int& fn)const{
        vn+=total_vertex_num_;
        fn+=total_face_num_;
    }

    
    void setObject(std::string filepath);

    void readObjFile(std::string inputfile="");

private:
    std::string filename_;
    tinyobj::ObjReader reader_;

    unsigned int total_shapes_;
    int total_vertex_num_=0;
    int total_face_num_=0;

    bool flip_normals_=false;
    bool back_culling_=true;

    std::unique_ptr<ObjectDesc> object_;
    std::vector<std::unique_ptr<Material>> all_mlts_;

};

