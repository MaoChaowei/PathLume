#pragma once
#include"common_include.h"
#include"algorithm"

class Texture{
public:
    // Texture():channel_num_(4){};
    Texture(std::string filename):channel_num_(4){
        loadFromFile(filename);
    }
    ~Texture();

    glm::vec4 operator[](uint32_t x);

    // return (1-t)color1+t*color2
    inline glm::vec4 learp(float t,const glm::vec4& color1,const glm::vec4& color2)const{
        return color1+t*(color2-color1);
    }

    void loadFromFile(std::string filename);


    glm::vec4 getColorBilinear(float x,float y);

private:
    int width_;
    int height_;
    int pnum_;
    const int channel_num_;
    unsigned char* data_;


};