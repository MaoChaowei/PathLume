#include"sample.h"

/*-----------------------------------------------------------*/
/*------------------------Sampler--------------------------*/
/*-----------------------------------------------------------*/
 /**
 * @brief inform sampler of the dimension of the array of sample
 */
uint32_t Sampler::preAddSamples1D(uint32_t dim_num){
    for(int i=0;i<dim_num;++i){
        samples1D_.push_back(std::vector<float>(spp_,0));
    }
    return samples1D_.size();
}

uint32_t Sampler::preAddSamples2D(uint32_t dim_num){
    for(int i=0;i<dim_num;++i){
        samples2D_.push_back(std::vector<glm::vec2>(spp_,glm::vec2(0.,0.)));
    }
    return samples2D_.size();
}

/**
 * @brief reset samples.
 * 
 */
void Sampler::startPixle(){
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
bool Sampler::nextPixleSample(){
    if(cur_sample_idx_>=spp_)   
        return false;

    ++cur_sample_idx_;
    cur_1Ddim_=cur_2Ddim_=0;

    return true;
}

/**
 * @brief Get the Sample1 D object. If samples1D has enough samples, we simply fetch one. Otherwise, degenerate into pcg random number generator.
 */
float Sampler::getSample1D(){
    assert(cur_sample_idx_<spp_);

    float ans=0;
    if(cur_1Ddim_<samples1D_.size()){
        ans=samples1D_[cur_1Ddim_][cur_sample_idx_];
    }else{
        ans=pcgRNG_.nextFloat();
    }
    ++cur_1Ddim_;
    return ans;
}

/**
 * @brief Get the Sample2 D object. If samples2D has enough samples, we simply fetch one. Otherwise, degenerate into pcg random number generator.
 */
glm::vec2 Sampler::getSample2D(){
    assert(cur_sample_idx_<spp_);

    glm::vec2 ans(0.);
    if(cur_2Ddim_<samples2D_.size()){
        ans=samples2D_[cur_2Ddim_][cur_sample_idx_];
    }else{
        float a=pcgRNG_.nextFloat();
        float b=pcgRNG_.nextFloat();
        ans=glm::vec2(a,b);
    }
    ++cur_2Ddim_;
    return ans;
}

void Sampler::generateSamples1D(uint32_t dim_idx){
    for(int i=0;i<spp_;++i){
        samples1D_[dim_idx][i]=pcgRNG_.nextFloat();
    }
}

void Sampler::generateSamples2D(uint32_t dim_idx){
    for(int i=0;i<spp_;++i){
        float a=pcgRNG_.nextFloat();
        float b=pcgRNG_.nextFloat();
        samples2D_[dim_idx][i]=glm::vec2(a,b);
    }
}


/*-----------------------------------------------------------*/
/*--------------------StratifiedSampler----------------------*/
/*-----------------------------------------------------------*/

void StratifiedSampler::generateSamples1D(uint32_t dim_idx){
    float inv_spp=1.0/spp_;
    for(int i=0;i<spp_;++i){
        float dt=jittered_flag_?pcgRNG_.nextFloat(-0.5,std::nextafter(0.5,1.0)):0.0;
        samples1D_[dim_idx][i]=(i+0.5+dt)*inv_spp;
    }
    std::shuffle(samples1D_[dim_idx].begin(),samples1D_[dim_idx].end(),*pcgRNG_.getRNG());
}

void StratifiedSampler::generateSamples2D(uint32_t dim_idx){
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

int StratifiedSampler::getUpperPerfectSquare(int n) {
    assert(n>0);
    int root = static_cast<int>(std::sqrt(n));
    if (root * root == n) 
        return n;
    
    return (root + 1) * (root + 1);
}