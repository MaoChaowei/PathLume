#include"utils.h"

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

}

std::ostream& operator<<(std::ostream& os, const AABB3d& aabb)
{
    os << "AABB3d { min = ("
       << aabb.min.x << ", " << aabb.min.y << ", " << aabb.min.z
       << "), max = ("
       << aabb.max.x << ", " << aabb.max.y << ", " << aabb.max.z
       << ") }\n";
    return os;
}