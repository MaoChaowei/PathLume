#pragma once
#include"common/common_include.h"
#include"common/utils.h"
#include <limits>

static const float OneMinusEpsilon = 0x1.fffffep-1;

class Sampler{
public:
    Sampler(uint32_t sample_per_pixel,uint64_t rng_seed=12):spp_(sample_per_pixel),pcgRNG_(rng_seed){}
    virtual ~Sampler(){}

    /**
     * @brief inform sampler the dimension of the array of sample
     */
    uint32_t preAddSamples1D(uint32_t dim_num){
        for(int i=0;i<dim_num;++i){
            samples1D_.push_back(std::vector<float>(spp_,0));
        }
        return samples1D_.size();
    }

    uint32_t preAddSamples2D(uint32_t dim_num){
        for(int i=0;i<dim_num;++i){
            samples2D_.push_back(std::vector<glm::vec2>(spp_,glm::vec2(0.,0.)));
        }
        return samples2D_.size();
    }

    /**
     * @brief reset samples.
     * 
     */
    void startPixle(){
        cur_sample_idx_=0;
        cur_1Ddim_=cur_2Ddim_=0;

        for(int i=0;i<samples1D_.size();++i){
            generateSamples1D(i);
        }
        for(int i=0;i<samples2D_.size();++i){
            generateSamples2D(i);
        }

    }

    /**
     * @brief move on to the next pixel sample
     */
    bool nextPixleSample(){
        if(cur_sample_idx_>=spp_)   
            return false;

        ++cur_sample_idx_;
        cur_1Ddim_=cur_2Ddim_=0;

        return true;
    }

    /**
     * @brief Get the Sample1 D object. If samples1D has enough samples, we simply fetch. Otherwise, degenerate into pcg random number generator.
     */
    float getSample1D(){
        assert(cur_sample_idx_<spp_);

        float ans=0;
        if(cur_1Ddim_<samples1D_.size()){
            ans=samples1D_[cur_1Ddim_][cur_sample_idx_];
        }else{
            ans=std::clamp((float)pcgRNG_.nextFloat(),(float)0.0,OneMinusEpsilon);
        }
        ++cur_1Ddim_;
        return ans;
    }

    /**
     * @brief Get the Sample2 D object. If samples2D has enough samples, we simply fetch. Otherwise, degenerate into pcg random number generator.
     */
    glm::vec2 getSample2D(){
        assert(cur_sample_idx_<spp_);

        glm::vec2 ans(0.);
        if(cur_2Ddim_<samples2D_.size()){
            ans=samples2D_[cur_2Ddim_][cur_sample_idx_];
        }else{
            float a=std::clamp((float)pcgRNG_.nextFloat(),(float)0.0,OneMinusEpsilon);
            float b=std::clamp((float)pcgRNG_.nextFloat(),(float)0.0,OneMinusEpsilon);
            ans=glm::vec2(a,b);
        }
        ++cur_2Ddim_;
        return ans;
    }



protected:

    // generate the `dim_idx` dimension of arrays of 1D sample
    virtual void generateSamples1D(uint32_t dim_idx){
        for(int i=0;i<spp_;++i){
            samples1D_[dim_idx][i]=std::clamp((float)pcgRNG_.nextFloat(),(float)0.0,OneMinusEpsilon);
        }
    }
    virtual void generateSamples2D(uint32_t dim_idx){
        for(int i=0;i<spp_;++i){
            float a=std::clamp((float)pcgRNG_.nextFloat(),(float)0.0,OneMinusEpsilon);
            float b=std::clamp((float)pcgRNG_.nextFloat(),(float)0.0,OneMinusEpsilon);
            samples2D_[dim_idx][i]=glm::vec2(a,b);
        }
    }

    std::vector<std::vector<float>> samples1D_;         // samples1D_[sample_dim][spp]
    std::vector<std::vector<glm::vec2>> samples2D_;
    uint32_t cur_sample_idx_; // the index of the current image sample
    uint32_t cur_1Ddim_;        // the current dimension of the array of samples
    uint32_t cur_2Ddim_;
    uint32_t spp_;
    PCGRandom pcgRNG_;
};

class StratifiedSampler:public Sampler{
public:
    StratifiedSampler(uint32_t sample_per_pixel,uint64_t rng_seed=12,bool jittered=true):Sampler(sample_per_pixel,rng_seed){
        spp_=getClosestPerfectSquare(spp_);
        jittered_flag_=jittered;
    }
    
protected:
    void generateSamples1D(uint32_t dim_idx) override{
        float inv_spp=1.0/spp_;
        for(int i=0;i<spp_;++i){
            float dt=jittered_flag_?pcgRNG_.nextFloat(-0.5,std::nextafter(0.5,1.0)):0.0;
            samples1D_[dim_idx][i]=(i+0.5+dt)*inv_spp;
        }
    }

    void generateSamples2D(uint32_t dim_idx) override{
        // stratified sampling
        int root=static_cast<int>(std::sqrt(spp_));
        float inv_root=1.0/root;
        for(int i=0;i<root;++i){
            for(int j=0;j<root;++j){
                float dx=jittered_flag_?pcgRNG_.nextFloat(-0.5,std::nextafter(0.5,1.0)):0.0;
                float dy=jittered_flag_?pcgRNG_.nextFloat(-0.5,std::nextafter(0.5,1.0)):0.0;
                float x=(j+0.5+dx)*inv_root;
                float y=(i+0.5+dy)*inv_root;

                samples2D_[dim_idx][i*root+j]=glm::vec2(x,y);
            }
        }
        // shuffle between image samples
        std::shuffle(samples2D_[dim_idx].begin(),samples2D_[dim_idx].end(),*pcgRNG_.getRNG());

    }


private:
    int getClosestPerfectSquare(int n) {
        assert(n>0);
        int root = static_cast<int>(std::sqrt(n));
        if (root * root == n) 
            return n;
        
        int lowerSquare = root * root;
        int upperSquare = (root + 1) * (root + 1);
        
        if (n - lowerSquare <= upperSquare - n)
            return lowerSquare;
        else
            return upperSquare;
    }

    bool jittered_flag_;

};