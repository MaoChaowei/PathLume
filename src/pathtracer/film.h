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
#include"tile.h"


struct TileMessageBlock{
    int cnt=0;
    std::vector<int> arr_check;
};

class Camera;

class Film{
public:
    Film(){};

    void initTiles(const RTracingSetting& setting,std::shared_ptr<ColorBuffer> buffer,const Scene* scene);

    int parallelTiles();



private:
    glm::vec2 resolution_;    // {width,height}
    glm::vec3 up_lt_pos_;
    glm::vec3 deltaX_;
    glm::vec3 deltaY_;
    uint32_t tile_num_;
    std::vector<std::unique_ptr<Tile>> tiles_;
    glm::vec3 camera_pos_;
    glm::vec3 camera_front_;// delete this
    
    // Critical Resources
    std::shared_ptr<TileMessageBlock> tile_msg_;    // use `mx_msg_` to avoid race.
    std::mutex mx_msg_;
    std::shared_ptr<PathTracer> tracer_;        // read only, thread safe
    TileInfo info_;

    friend Camera;
    friend Tile;
};

