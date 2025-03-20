#pragma once
#include"common_include.h"

class ColorBuffer{
public:
    
    ColorBuffer()=delete;
    ColorBuffer(const ColorBuffer& other)=delete;
    ColorBuffer& operator=(const ColorBuffer& other)=delete;
    ColorBuffer(ColorBuffer&&) = delete;
    ColorBuffer& operator=(ColorBuffer&&) = delete;

    ColorBuffer(int width,int height):width_(width),height_(height){
        addr_=new unsigned char[width*height*4];
        pixel_num_=width*height;
    }

    void clear(const unsigned char num=0){
        std::fill(addr_,addr_+width_*height_*4,num);
    }
    void reSetBuffer(int width,int height){
        width_=width;
        height_=height;
        delete []addr_;
        addr_=new unsigned char[width*height*4];
        pixel_num_=width*height;
    }
    unsigned char* getAddr()const{return addr_;}
    uint32_t getPixelNum()const{return pixel_num_;}

    // make sure that x and y must in the window
    /**
     * @brief Set the Pixel color in color buffer
     * 
     * @param x 
     * @param y 
     * @param color must in device space ranging in[0~255]
     */
    void setPixel(int x,int y,const glm::vec4& color)const{
        int idx=y*width_+x;
        if(idx>=pixel_num_||idx<0) 
            return;

        for(int i=0;i<4;++i)
            addr_[idx*4+i]=color[i];
    }

    ~ColorBuffer(){
        delete[]addr_;
    }
    
    bool saveToImage(const std::string& filename) const;

private:
    int width_;
    int height_;
    // addr_:帧缓存起始地址，采用rgba四字节连续存储的方式表示一个像素的颜色，
    // viewport的地址（x，y）按照：左下角为（0,0），右上角为（width-1,height-1）
    unsigned char* addr_;
    int pixel_num_=0;

}; 

// forward declair
class HZbuffer;

class DepthBuffer{
public:
    DepthBuffer(){};
    DepthBuffer(int width,int height){
        width_=width;
        height_=height;
        pixel_num_=width*height;
        zbuffer_.resize(width*height);
        std::fill(zbuffer_.begin(),zbuffer_.end(),1.0f);
    }
    inline void reSetBuffer(int width,int height){
        width_=width;
        height_=height;
        pixel_num_=width*height;
        zbuffer_.resize(width*height);
        std::fill(zbuffer_.begin(),zbuffer_.end(),1.0f);
    }

    void clear(){
        // 1.0F is the farest in NDC space
        std::fill(zbuffer_.begin(),zbuffer_.end(),1.0f);
    }

    void setDepth(int x,int y,float depth){
        int idx=y*width_+x;
        if(idx>=pixel_num_){
            std::cerr<<"setDepth: (x,y) out of range!x="<<x<<"/"<<width_-1<<",y="<<y<<"/"<<height_-1<<std::endl;
            exit(-1);
        }
        zbuffer_[idx]=depth;
    }

    inline std::shared_ptr<std::vector<float>> getZbuffer(){
        return std::make_shared<std::vector<float>>(zbuffer_);
    }

    /**
     * depth test on (x,y): check wheather current z-depth of (x,y) is closer than `depth`. if so,
     * fail the depth test and return False, on the contrary, update z-buffer and return True. 
     */
    inline bool zTest(int x,int y,float depth){
        int idx=y*width_+x;
        if(x<0||y<0||x>width_-1||y>height_-1){
            std::cerr<<"zTest: (x,y) out of range!x="<<x<<"/"<<width_-1<<",y="<<y<<"/"<<height_-1<<std::endl;
            return false;
        }

        if(depth>zbuffer_[idx]) 
            return false;

        zbuffer_[idx]=depth;
        return true;
    }

    inline float getDepth(int x,int y){
        int idx=y*width_+x;
        if(x<0||y<0||x>width_-1||y>height_-1){
            std::cerr<<"getDepth: (x,y) out of range!x="<<x<<"/"<<width_-1<<",y="<<y<<"/"<<height_-1<<std::endl;
            return 0;// exit(-1);
        }
        return zbuffer_[idx];
    }

private:
    int width_;
    int height_;
    int pixel_num_;
    std::vector<float> zbuffer_;

    friend class HZbuffer;
};
 