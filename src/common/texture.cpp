#include"texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


glm::vec4 Texture::getColorBilinear(float x,float y){
    x=std::clamp(x,0.0f,1.0f);
    y=std::clamp(y,0.0f,1.0f);
    float img_x=x*(width_-1);
    float img_y=y*(height_-1);

    int Cx = (int)img_x;
    int Cy = (int)img_y;

    float fx = img_x - Cx;
    float fy = img_y - Cy;

    int Cx1 = std::min(Cx+1, width_-1); // in case c01 and c11 are out of  width
    int Cy1 = std::min(Cy+1, height_-1);

    int u00 = Cx     + Cy * width_;
    int u10 = Cx1    + Cy * width_;
    int u01 = Cx     + Cy1 * width_;
    int u11 = Cx1    + Cy1 * width_;

    glm::vec4 C00 = (*this)[u00];
    glm::vec4 C10 = (*this)[u10];
    glm::vec4 C01 = (*this)[u01];
    glm::vec4 C11 = (*this)[u11];

    glm::vec4 C_low    = learp(fx, C00, C10);
    glm::vec4 C_up = learp(fx, C01, C11);
    
    return learp(fy, C_low, C_up);
}


Texture::~Texture(){
    if(data_){
        stbi_image_free(data_);
        data_=nullptr;
    }
}

glm::vec4 Texture::operator[](uint32_t x){
    if(x>=pnum_){
        std::cout<<"getColor: out of range!\n";
        exit(-1);
    }
    int idx=x*channel_num_;
    return glm::vec4(data_[idx],data_[idx+1],data_[idx+2],data_[idx+3]);
}


void Texture::loadFromFile(std::string filename){
    int channel;
    stbi_set_flip_vertically_on_load(1);
    data_=stbi_load(filename.c_str(),&width_,&height_,&channel,channel_num_);
    if(!data_){
        std::cout<<"Try to search from the upper level directory..."<<std::endl;
        filename = "../"+filename;
        data_=stbi_load(filename.c_str(),&width_,&height_,&channel,channel_num_);
        if(!data_){
            std::cout<<"Failed to load texture: "<<filename<<",please check the file path of this texture.\n";
        }
    }

    if(data_){
        std::cout<<"Successfully load Texture: "<<filename<<".\n";
    }
    
    pnum_=width_*height_;
}