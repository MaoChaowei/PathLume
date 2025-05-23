# pragma once
#include <random>
#include"common_include.h"
#include"AABB.h"
#include <fstream>


// some small functions
namespace utils{

// get the barycenter of goal_p in the p1-p2-p3 triangle
inline glm::vec3 getBaryCenter(const glm::vec2 p1, const glm::vec2 p2, const glm::vec2 p3, const glm::vec2 goal_p) {
    float denom = (p2.y - p3.y) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.y - p3.y);
    if (std::abs(denom) < 1e-6) {
        // throw std::runtime_error("Triangle is degenerate, denominator is zero.");
        return glm::vec3(-1,-1,-1);
    }
    float lambda1 = ((p2.y - p3.y) * (goal_p.x - p3.x) + (p3.x - p2.x) * (goal_p.y - p3.y)) / denom;
    float lambda2 = ((p3.y - p1.y) * (goal_p.x - p3.x) + (p1.x - p3.x) * (goal_p.y - p3.y)) / denom;
    float lambda3 = 1.0f - lambda1 - lambda2;

    return glm::vec3(lambda1, lambda2, lambda3);
}

void printvec3(glm::vec3 v,std::string str);
void lowerVec3(glm::vec3& v);
void upperVec3(glm::vec3& v);

bool isEqual(float a, float b, float eps=srender::EPSILON);


std::string getCurrentTime();

/**
 * @brief DEBUG to file
 */
template<typename... Args>
void DebugLog(const std::string& format, Args&&... args)
{
    static std::ofstream g_logFile("debug.log", std::ios::out | std::ios::app);

    size_t size = snprintf(nullptr, 0, format.c_str(), args...) + 1;
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format.c_str(), args...);

    std::string output = buf.get();

    // std::cout << output << std::endl;

    if (g_logFile.is_open())
    {
        g_logFile << output << std::endl;
        g_logFile.flush();
    }
}

inline float getLuminance(const glm::vec3 radiance_rgb){
    return (0.2126*radiance_rgb.r + 0.7152*radiance_rgb.g + 0.0722*radiance_rgb.b);
}

inline glm::vec3 srgbToLinear(const glm::vec3& srgb) {
    glm::vec3 linear;
    for(int i=0;i<3;++i){
        linear[i]=std::pow(srgb[i],2.2f);
    }
    return linear;
}

}

std::ostream& operator<<(std::ostream& os, const AABB3d& aabb);

// random number generator

class PCGRandom {
public:
    // use random seed
    PCGRandom() : rng(rd()) {}  
    // specify seed
    explicit PCGRandom(uint64_t seed) : rng(seed) {}

    // get random integer (0 - MAX_UINT64)
    uint64_t nextInt() {
        return rng();
    }

    // get random integer [min, max]
    uint64_t nextInt(uint64_t min, uint64_t max) {
        std::uniform_int_distribution<uint64_t> dist(min, max);
        return dist(rng);
    }

    // get random double [min, max)
    float nextFloat(float min=0.0, float max=1.0) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(rng);
    }

    std::mt19937_64* getRNG(){return &rng;}

private:
    std::random_device rd; // get random seed
    std::mt19937_64 rng; 
};
