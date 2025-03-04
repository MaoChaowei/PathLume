#pragma once
#include<iostream>
#include<vector>
#include<string>
#include<unordered_map>
#include<map>
#include<utility>   // std::pair

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtc/constants.hpp> // glm::pi
#include<memory>
#include<math.h>
#include<algorithm>
#include<limits>

#define TIME_RECORD // a switch of time recording


namespace srender{
    
// 定义浮点数比较的容差
const float EPSILON = 1e-6f;
const int INF = 2147483647;
const float NEAR_Z=-1;
const float FAR_Z=1;
const float MAXFLOAT=std::numeric_limits<float>::max();

}