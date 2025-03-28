#pragma once
#include"common/common_include.h"
#include"common/utils.h"
#include <limits>

/**
 * @brief Sampler takes charge of generating 1D/2D samples for a single pixel.
 * Samples generated are ought to distribute among [0,1] uniformly.More advanced
 * sampling techniques will be implemented in Sampler's derivative class.
 * 
 */
class Sampler{
public:
    Sampler(uint32_t sample_per_pixel,uint64_t rng_seed=12):spp_(sample_per_pixel),pcgRNG_(rng_seed){}
    virtual ~Sampler(){}

    /**
     * @brief inform sampler of the dimension of the array of sample
     */
    uint32_t preAddSamples1D(uint32_t dim_num);

    uint32_t preAddSamples2D(uint32_t dim_num);

    /**
     * @brief reset samples.
     * 
     */
    void startPixle();

    /**
     * @brief move on to the next pixel sample
     */
    bool nextPixleSample();

    /**
     * @brief Get the Sample1 D object. If samples1D has enough samples, we simply fetch one. Otherwise, degenerate into pcg random number generator.
     */
    float getSample1D();

    /**
     * @brief Get the Sample2 D object. If samples2D has enough samples, we simply fetch one. Otherwise, degenerate into pcg random number generator.
     */
    glm::vec2 getSample2D();


    PCGRandom pcgRNG_;
    
protected:

    // generate the `dim_idx` dimension of arrays of 1D sample
    virtual void generateSamples1D(uint32_t dim_idx);
    virtual void generateSamples2D(uint32_t dim_idx);

    std::vector<std::vector<float>> samples1D_;         // samples1D_[sample_dim][spp]
    std::vector<std::vector<glm::vec2>> samples2D_;
    uint32_t cur_sample_idx_; // the index of the current image sample
    uint32_t cur_1Ddim_;        // the current dimension of the array of samples
    uint32_t cur_2Ddim_;
    uint32_t spp_;
    // PCGRandom pcgRNG_;
};

class StratifiedSampler:public Sampler{
public:
    StratifiedSampler(uint32_t sample_per_pixel,uint64_t rng_seed=12,bool jittered=true):Sampler(sample_per_pixel,rng_seed){
        spp_=getUpperPerfectSquare(spp_);
        jittered_flag_=jittered;
    }
    
protected:
    void generateSamples1D(uint32_t dim_idx) override;
    void generateSamples2D(uint32_t dim_idx) override;


private:
    int getUpperPerfectSquare(int n);

    bool jittered_flag_;

};