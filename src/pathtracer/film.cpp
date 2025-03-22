#include"film.h"
#include"tile.h"

void Film::initTiles(const RTracingSetting& setting,std::shared_ptr<ColorBuffer> buffer,const Scene* scene){

    // init shared memory
    tracer_=std::make_shared<PathTracer>();
    // tracer_=std::make_shared<MonteCarloPathTracer>(setting.max_depth_);
    // tracer_=std::make_shared<MonteCarloPathTracerNEE>(setting.max_depth_);
    
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
            tiles_.emplace_back(std::make_unique<Tile>(j,this,px_num,px_offset,pos,buffer,scene,tracer_,setting));
        }
    }


    // init sampler
    for(int i=0;i<tiles_.size();++i){
        tiles_[i]->sampler_=std::make_unique<StratifiedSampler>(setting.spp_, i,true);
        tiles_[i]->sampler_->preAddSamples2D(1+2*10);    // image samples,each path sample a Wi
        tiles_[i]->sampler_->preAddSamples1D(1+1*10); // each path sample a emitter
    }
}

int Film::parallelTiles(){

    /*
    // VERSION 1: didn't control the number of threads  
    std::vector<std::thread> threads;
    for(int i=0;i<tile_num_*tile_num_;++i){
        threads.emplace_back(std::thread(&Tile::render, tiles_[i].get()));
    }

    for(auto& t:threads)
        t.join();

    */

    // get system's max concurrency
    size_t threadCnt=std::thread::hardware_concurrency()-1;
    std::cout<<"System's max concurrency is "<<threadCnt+1<<std::endl;
    
    size_t total_tiles=tile_num_*tile_num_;
    threadCnt = std::min(threadCnt,total_tiles);
    if(threadCnt==0){
        threadCnt=8;
    }
    
    std::vector<std::thread> threads_pool;
    threads_pool.reserve(threadCnt);

    // dispatch assignments to each logic thread
    for(size_t t=0;t<threadCnt;++t){
        threads_pool.emplace_back(
            [&,t]{
                for(size_t i=t;i<total_tiles;i+=threadCnt){
                    tiles_[i]->render();
                }
            }
        );
        // threads_pool.back().join();
        // std::cout<<"finish t="<<t<<std::endl;
    }

    for(auto& th:threads_pool)
        th.join();

    for(auto& tile:tiles_){
        info_.avg_length+=tile->info_.avg_length;
    }
    info_.avg_length/=tiles_.size();
    std::cout<<"Average depth is : "<<info_.avg_length<<std::endl;

#ifdef THREAD_SAFTY_CHECK
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
#endif

    return threadCnt;
    
}


