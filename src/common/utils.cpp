#include"utils.h"
#include<chrono>
#include<ctime>
#include <sstream>
#include<iomanip>

// some small functions
namespace utils{

    void printvec3(glm::vec3 v,std::string str=""){
        if(str.size()) std::cout<<str;
        std::cout<<": { "<<v.x<<", "<<v.y<<", "<<v.z<<"}\n";
    }
    void lowerVec3(glm::vec3& v){
        v.x=(int)v.x;
        v.y=(int)v.y;
        v.z=(int)v.z;
    }

    void upperVec3(glm::vec3& v){
        v.x=(int)v.x+1;
        v.y=(int)v.y+1;
        v.z=(int)v.z+1;
    }

    bool isEqual(float a, float b, float eps){
        return abs(a-b)<eps;
}

std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* local_tm = std::localtime(&now_time);

    std::ostringstream oss;
    oss << std::put_time(local_tm, "%H_%M"); // 格式化为 "HHMM"
    
    return oss.str();
}

}// end of namespace

std::ostream& operator<<(std::ostream& os, const AABB3d& aabb)
{
    os << "AABB3d { min = ("
       << aabb.min.x << ", " << aabb.min.y << ", " << aabb.min.z
       << "), max = ("
       << aabb.max.x << ", " << aabb.max.y << ", " << aabb.max.z
       << ") }\n";
    return os;
}