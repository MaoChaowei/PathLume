#include"object.h"
#include <iomanip>
#include<glm/gtx/hash.hpp>
#include"algorithm"
#include"utils.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace object_tools{

Vertex extractVertex(const tinyobj::attrib_t& attrib,const tinyobj::index_t idx){
    Vertex temp;
    // access to vertex
    tinyobj::real_t vx = attrib.vertices[3*size_t(idx.vertex_index)+0];
    tinyobj::real_t vy = attrib.vertices[3*size_t(idx.vertex_index)+1];
    tinyobj::real_t vz = attrib.vertices[3*size_t(idx.vertex_index)+2];
    temp.pos_=glm::vec3(vx,vy,vz);


    // negative = no normal data
    if (idx.normal_index >= 0) {
        tinyobj::real_t nx = attrib.normals[3*size_t(idx.normal_index)+0];
        tinyobj::real_t ny = attrib.normals[3*size_t(idx.normal_index)+1];
        tinyobj::real_t nz = attrib.normals[3*size_t(idx.normal_index)+2];
        temp.norm_=glm::vec3(nx,ny,nz);
    }

    // negative = no texcoord data
    if (idx.texcoord_index >= 0) {
        tinyobj::real_t tx = attrib.texcoords[2*size_t(idx.texcoord_index)+0];
        tinyobj::real_t ty = attrib.texcoords[2*size_t(idx.texcoord_index)+1];
        temp.uv_=glm::vec2(tx,ty);
    }

    //vertex colors
    tinyobj::real_t red   = std::min(attrib.colors[3*size_t(idx.vertex_index)+0]*255.0,255.0);
    tinyobj::real_t green = std::min(attrib.colors[3*size_t(idx.vertex_index)+1]*255.0,255.0);
    tinyobj::real_t blue  = std::min(attrib.colors[3*size_t(idx.vertex_index)+2]*255.0,255.0);
    temp.color_=glm::vec4(red,green,blue,1.0f);

    return temp;
 }

}



//---------------------Mesh--------------------------//

void Mesh::initObject(const tinyobj::ObjReader& reader,std::string filepath,bool flip_normals,bool backculling){
    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& mtls = reader.GetMaterials();
    name_=shapes[0].name;
    face_num_=0;
    do_back_culling_=backculling;
   
    std::unordered_map<Vertex,uint32_t> ver2idx;
    has_normal_=has_uv_=true;

    // initialize all the vetices 
    for(auto& info:shapes){
        face_num_+=info.mesh.num_face_vertices.size();
        for(auto& idx:info.mesh.indices){
            Vertex v=object_tools::extractVertex(attrib,idx);
            if(ver2idx.find(v)!=ver2idx.end()){
                // already preserved
                indices_.push_back(ver2idx[v]);
            }
            else{
                uint32_t t=vertices_.size();
                vertices_.push_back(v);
                indices_.push_back(t);
                ver2idx[v]=t;
            }
            if(has_normal_&&idx.normal_index<0) has_normal_=false;
            if(has_uv_&&idx.texcoord_index<0) has_uv_=false;
        }
        for(auto& idx:info.mesh.material_ids){
            mtlidx_.push_back(idx);
        }
    }
    // initialize all the materials
    size_t pos= filepath.find_last_of("/\\");
    std::string prefix=filepath.substr(0,pos)+"/";
    for(auto& m:mtls){
        std::shared_ptr<Material> mptr=std::make_shared<Material>();
        mptr->setName(m.name);
        mptr->setADS(glm::vec3(m.ambient[0],m.ambient[1],m.ambient[2]),
                     glm::vec3(m.diffuse[0],m.diffuse[1],m.diffuse[2]),
                     glm::vec3(m.specular[0],m.specular[1],m.specular[2]));

        if(m.ambient_texname.size())    mptr->setTexture(MltMember::Ambient,prefix+m.ambient_texname);
        if(m.diffuse_texname.size())    mptr->setTexture(MltMember::Diffuse,prefix+m.diffuse_texname);
        if(m.specular_texname.size())    mptr->setTexture(MltMember::Specular,prefix+m.specular_texname);

        mtls_.push_back(mptr);
    }

    int sign=flip_normals?-1:1;
    // calculate face and vertex normals
    for(int i=0;i<face_num_;++i){
        glm::vec3 a=vertices_[indices_[i*3+0]].pos_;
        glm::vec3 b=vertices_[indices_[i*3+1]].pos_;
        glm::vec3 c=vertices_[indices_[i*3+2]].pos_;
        glm::vec3 cb=b-c;
        glm::vec3 ba=a-b;
        glm::vec3 nn=glm::vec3(sign)*glm::cross(ba,cb);

        face_normals_.push_back(nn);
        if(!has_normal_){
            for(int t=0;t<3;++t){
                vertices_[indices_[i*3+t]].norm_+=face_normals_[i];
            }
        }
    }
    for(auto& v:vertices_){
        v.norm_=glm::normalize(v.norm_);
    }
    has_normal_=true;
}


void Mesh::printInfo() const {
    std::cout << "Mesh Name: " << name_ << std::endl;
    std::cout << "Number of Vertices: " << vertices_.size() << std::endl;
    std::cout << "Number of Indices: " << indices_.size() << std::endl;
    std::cout << "Number of Faces (Triangles): " << indices_.size() / 3 << std::endl;

    // 打印顶点信息
    std::cout << "\nVertices:" << std::endl;
    for (size_t i = 0; i < vertices_.size(); ++i) {
        std::cout << "Vertex " << i << ": " << vertices_[i] << std::endl;
    }

    // 打印索引信息并还原三角形面
    std::cout << "\nTriangles:" << std::endl;
    size_t triangleCount = indices_.size() / 3;
    for (size_t i = 0; i < triangleCount; ++i) {
        uint32_t index0 = indices_[i * 3 + 0];
        uint32_t index1 = indices_[i * 3 + 1];
        uint32_t index2 = indices_[i * 3 + 2];

        // 输出三角形的索引
        std::cout << "Triangle " << i << ": Indices(" << index0 << ", " << index1 << ", " << index2 << ")" << std::endl;

        // 输出组成三角形的顶点信息
        std::cout << "    Vertex 0: " << vertices_[index0] << std::endl;
        std::cout << "    Vertex 1: " << vertices_[index1] << std::endl;
        std::cout << "    Vertex 2: " << vertices_[index2] << std::endl;
    }
}




//---------------------Line--------------------------//

void Line::printInfo() const {
    std::cout << "Line Name: " << name_ << std::endl;
    std::cout << "Number of Vertices: " << vertices_.size() << std::endl;
    std::cout << "Number of Indices: " << indices_.size() << std::endl;
    std::cout << "Number of Lines: " << indices_.size()+1 << std::endl;

    // 打印顶点信息
    std::cout << "\nVertices:" << std::endl;
    for (size_t i = 0; i < vertices_.size(); ++i) {
        std::cout << "Vertex " << i << ": " << vertices_[i] << std::endl;
    }
}

//---------------------ObjLoader--------------------------//

/**
 * @brief banking on `tiny-objloader` to finish obj-file resolving job~
 * everything is conserved in `reader_`
 * 
 * @param inputfile : relative searching route to obj file.
 */
void ObjLoader::readObjFile(std::string inputfile){

    if(inputfile.empty())
        inputfile=filename_;

    std::cout<<"Obj-Loader: Searching "<< inputfile <<std::endl;

    if (!reader_.ParseFromFile(inputfile)) {

        if (!reader_.Error().empty()) {
            std::cerr << "Obj-Loader: " << reader_.Error();
            std::cout<<"try to search ../assets/model/"<<std::endl;
        }

        inputfile = "../assets/model/cornell_box.obj";
        if (!reader_.ParseFromFile(inputfile)) {
            if (!reader_.Error().empty()) {
                std::cerr << "Obj-Loader: " << reader_.Error();
            }
            exit(1);
        }
    }
    if (!reader_.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader_.Warning();
    }

    std::cout<<"Obj-Loader: Successfully found .obj "<<std::endl;
}

/**
 * @brief retrive everything in `reader_` to my structure for further usage
 * 
 */
void ObjLoader::setObject(std::string filepath){
    auto mesh = std::make_unique<Mesh>();   // instance of the derived class
    mesh->initObject(reader_,filepath,flip_normals_,back_culling_);

    total_face_num_=mesh->getFaceNum();
    total_vertex_num_=mesh->getVerticesNum();

    if(mesh->getVerticesNum()>0)
        object_=std::move(mesh);
    
}


