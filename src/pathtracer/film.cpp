#include"film.h"

void Tile::render(){

    for(int j=0;j<pixels_num_.y;++j){
        for(int i=0;i<pixels_num_.x;++i){
            glm::vec3 color(0);
            sampler_->startPixle();
            for(int s=0;s<setting_.spp_;++s){
                // generate a ray
                glm::vec2 offset=sampler_->getSample2D();
                glm::vec3 origin=film_->camera_pos_;
                glm::vec3 sample_pos=up_lt_pos_+float(i)*film_->deltaX_+float(j)*film_->deltaY_;
                glm::vec3 direction=sample_pos-origin;
                float startT=srender::EPSILON;
                float endT=srender::MAXFLOAT;

                Ray ray(origin,direction,startT,endT);
                // trace the ray and get its color
                color+=tracer_->traceRay(ray,scene_);
                
                // move on to the next image sample.
                sampler_->nextPixleSample();
            }

            // set color to buffer
            color=(float)(1.0/setting_.spp_)*color;
            setPixel(i,j,glm::vec4(color,1.0));

        }
    }
}

void Tile::setPixel(const uint32_t x,const uint32_t y,const glm::vec4& color){
    int offset_x=first_pixel_offset_.x+x;
    int offset_y=first_pixel_offset_.y+y;
    // Since the origin of color_buffer is bottom-left, a simple transformation is going on here~ 
    shared_buffer_->setPixel(offset_x,film_->resolution_.y-1-offset_y,color);

    {
        std::lock_guard<std::mutex> lock(film_->mx_msg_);
        ++film_->tile_msg_->cnt;
        ++film_->tile_msg_->arr_check[offset_x+offset_y*film_->resolution_.x];
        // std::cout<<"offset_x: "<<offset_x<<" , offset_y:"<<offset_y<<std::endl;
    }
}