#pragma once
#include"common_include.h"
#include"scene_loader.h"
#include"interface.h"
#include"sample.h"
#include"pathtracer.h"
#include"buffer.h"

class Film;

struct TileInfo{
    uint32_t avg_length=0;
};

class Tile{
public:
    Tile()=delete;
    Tile(uint32_t idx,Film* film,glm::vec2 pnum,glm::vec2 px_offset,glm::vec3 up_lt,std::shared_ptr<ColorBuffer> buffer,
        const Scene* scene,std::shared_ptr<PathTracer>pathtracer,const RTracingSetting& setting)
        :tile_idx_(idx),film_(film),pixels_num_(pnum),first_pixel_offset_(px_offset),up_lt_pos_(up_lt),shared_buffer_(buffer),
        scene_(scene),tracer_(pathtracer),setting_(setting){}

    void render();
    void setPixel(const uint32_t x,const uint32_t y,const glm::vec4& linear_color);


private:
    uint32_t tile_idx_;
    glm::vec2 pixels_num_; //{width,height}
    glm::vec2 first_pixel_offset_; // the left_top pixel's index in the whole film.
    glm::vec3 up_lt_pos_;          // the left_top pixel's left top corner's world position.
    Film* film_;
    const Scene* scene_;
    const RTracingSetting& setting_;
    std::shared_ptr<ColorBuffer> shared_buffer_;
    std::shared_ptr<PathTracer> tracer_;
    std::unique_ptr<Sampler> sampler_;
    TileInfo info_;

    friend Film;
};