#include"tile.h"
#include"film.h"


void Tile::render(){

    info_.avg_length=0;

    for(int j=0;j<pixels_num_.y;++j){
        for(int i=0;i<pixels_num_.x;++i){
            glm::vec3 color(0);
            sampler_->startPixle();
            for(int s=0;s<setting_.spp_;++s){
                // generate a ray
                glm::vec2 offset=sampler_->getSample2D();
                glm::vec3 origin=film_->camera_pos_;
                glm::vec3 sample_pos=up_lt_pos_+float(i+offset.x)*film_->deltaX_+float(j+offset.y)*film_->deltaY_;
                glm::vec3 direction=sample_pos-origin;
                float startT=srender::EPSILON;
                float endT=srender::MAXFLOAT;

                Ray ray(origin,direction,startT,endT);
                // trace the ray and get its color
                PathTraceRecord pRec(*scene_,*sampler_,setting_.light_split_);
                color+=tracer_->Li(ray,pRec);
                info_.avg_length+=pRec.curdepth;
                
                // move on to the next image sample.
                sampler_->nextPixleSample();

            }

            // set color to buffer
            color=(float)(1.0/setting_.spp_)*color;
            setPixel(i,j,glm::vec4(color,1.0));

        }
    }

    // do some statistics
    info_.avg_length/=(pixels_num_.x*pixels_num_.y*setting_.spp_);

}

/**
 * @brief implement post color process and write the device rgb color to buffer
 * 
 * @param x 
 * @param y 
 * @param linear_color is the color in linear-space ranging from 0~1.
 */
void Tile::setPixel(const uint32_t x,const uint32_t y,const glm::vec4& linear_color){

    /*******************POST PROCESS********************* */
    glm::vec3 gammacolor=linear_color;
    for(int i=0;i<3;++i){
        gammacolor[i]=255.f*pow(std::clamp(gammacolor[i],0.f,0.9999999999f),1.0/2.2);
    }

    /***************************************************** */
    int offset_x=first_pixel_offset_.x+x;
    int offset_y=first_pixel_offset_.y+y;

    // Since the origin of color_buffer is bottom-left and that of film is top-left,
    // a simple transformation is going on here
    shared_buffer_->setPixel(offset_x,film_->resolution_.y-1-offset_y,glm::vec4(gammacolor,255.f));

    // check thread safty when necessary
#ifdef THREAD_SAFTY_CHECK
    {
        std::lock_guard<std::mutex> lock(film_->mx_msg_);
        ++film_->tile_msg_->cnt;
        ++film_->tile_msg_->arr_check[offset_x+offset_y*film_->resolution_.x];
    }
#endif
}