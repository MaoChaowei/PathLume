#pragma once
#include"common/common_include.h"
#include"buffer.h"
#include <chrono>
#include <thread>
#include <mutex>
#include"scene_loader.h"
#include"interface.h"
#include"sample.h"
#include"pathtracer.h"

struct TileMessageBlock{
    int cnt=0;
    std::vector<int> arr_check;
};

class Film;
class Camera;

class Tile{
public:
    Tile()=delete;
    Tile(Film* film,glm::vec2 pnum,glm::vec2 px_offset,glm::vec3 up_lt,std::shared_ptr<ColorBuffer> buffer,
        const Scene* scene,std::shared_ptr<const PathTracer>pathtracer,const RTracingSetting& setting)
        :film_(film),pixels_num_(pnum),first_pixel_offset_(px_offset),up_lt_pos_(up_lt),shared_buffer_(buffer),
        scene_(scene),tracer_(pathtracer),setting_(setting){}

    void render();
    void setPixel(const uint32_t x,const uint32_t y,const glm::vec4& color);

private:
    glm::vec2 pixels_num_; //{width,height}
    glm::vec2 first_pixel_offset_; // the left_top pixel's index in the whole film.
    glm::vec3 up_lt_pos_;          // the left_top pixel's left top corner's world position.
    Film* film_;
    const Scene* scene_;
    const RTracingSetting& setting_;
    std::shared_ptr<ColorBuffer> shared_buffer_;
    std::shared_ptr<const PathTracer> tracer_;
    std::unique_ptr<Sampler> sampler_;
    
    friend Film;
};

class Film{
public:
    Film(){};

    void initTiles(const RTracingSetting& setting,std::shared_ptr<ColorBuffer> buffer,const Scene* scene){

        // init shared memory
        tracer_=std::make_shared<PathTracer>();
        tile_msg_=std::make_shared<TileMessageBlock>();
        tile_msg_->arr_check.resize(buffer->getPixelNum());

        // init tiles
        tile_num_=setting.tiles_num_;
        assert(buffer->getPixelNum()==resolution_.x*resolution_.y);

        // each tile's pixel num
        int w=(resolution_.x+tile_num_-1)/tile_num_;
        int h=(resolution_.y+tile_num_-1)/tile_num_;
        // each tile's physical size
        glm::vec3 vec_w=float(w)*deltaX_;
        glm::vec3 vec_h=float(h)*deltaY_;
        
        // each row
        for(int i=0;i<tile_num_;++i){
            
            int px_h=(i==tile_num_-1)?resolution_.y-(tile_num_-1)*h:h;
            
            // each column
            for(int j=0;j<tile_num_;++j){
                int px_w=(j==tile_num_-1)?resolution_.x-(tile_num_-1)*w:w;

                glm::vec2 px_num(px_w,px_h);
                glm::vec3 pos=up_lt_pos_+float(i)*vec_h+float(j)*vec_w;
                glm::vec2 px_offset(j*w,i*h);
                tiles_.emplace_back(std::make_unique<Tile>(this,px_num,px_offset,pos,buffer,scene,tracer_,setting));
            }
        }


        // init sampler
        for(int i=0;i<tiles_.size();++i){
            tiles_[i]->sampler_=std::make_unique<StratifiedSampler>(setting.spp_, i,true);
            tiles_[i]->sampler_->preAddSamples2D(1);    // image samples
            tiles_[i]->sampler_->preAddSamples1D(1);    // test samples
        }
    }

    void parallelTiles(){
        auto begin = std::chrono::system_clock::now();

        std::vector<std::thread> threads;
        for(int i=0;i<tile_num_*tile_num_;++i){
            threads.emplace_back(std::thread(&Tile::render, tiles_[i].get()));
        }

        for(auto& t:threads)
            t.join();

        auto end = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end-begin);

        bool pass=true;
        if(tile_msg_->cnt!=resolution_.x*resolution_.y){
            std::cout<<"err: tile_msg_.cnt = "<<tile_msg_->cnt<<";resolution_.x*resolution_.y="<<resolution_.x*resolution_.y<<std::endl;
            pass=false;
        }
        for(int i=0;i<resolution_.x*resolution_.y;++i){
            if(tile_msg_->arr_check[i]!=1){
                std::cout<<"err: tile_msg_->arr_check["<<i<<"] != 1 "<<std::endl;
                pass=false;
            }
            
        }
        pass==true?std::cout<<"pass= true\n":std::cout<<"pass= false\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout<<"duration="<<duration.count()/1000.0<<std::endl;

        
    }



private:
    glm::vec2 resolution_;    // {width,height}
    glm::vec3 up_lt_pos_;
    glm::vec3 deltaX_;
    glm::vec3 deltaY_;
    uint32_t tile_num_;
    std::vector<std::unique_ptr<Tile>> tiles_;

    glm::vec3 camera_pos_;
    
    // Critical Resource
    std::shared_ptr<TileMessageBlock> tile_msg_;    // use `mx_msg_` to avoid race.
    std::mutex mx_msg_;
    std::shared_ptr<PathTracer> tracer_;        // read only, thread safe

    friend Camera;
    friend Tile;
};