#include"film.h"

void Tile::render(){
    for(int j=0;j<pixels_num_.y;++j){
        for(int i=0;i<pixels_num_.x;++i){
            int offset_x=first_pixel_offset_.x+i;
            int offset_y=first_pixel_offset_.y+j;
            // Since the origin of color_buffer is bottom-left, a simple transformation is going on here~ 
            shared_buffer_->setPixel(offset_x,film_->resolution_.y-1-offset_y,glm::vec4(127,0,0,1));

            // std::this_thread::sleep_for(std::chrono::microseconds(1000));
            {
                std::lock_guard<std::mutex> lock(film_->mx_msg_);
                ++film_->tile_msg_->cnt;
                ++film_->tile_msg_->arr_check[offset_x+offset_y*film_->resolution_.x];
                // std::cout<<"offset_x: "<<offset_x<<" , offset_y:"<<offset_y<<std::endl;
            }
        }
    }
}