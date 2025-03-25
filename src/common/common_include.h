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
#include <stdexcept>

#define TIME_RECORD // a switch of time recording
// #define THREAD_SAFTY_CHECK
// #define DEBUG_MODE

namespace srender{
    
// 定义浮点数比较的容差
constexpr float EPSILON = 1e-6f;
constexpr float AABBOX_EPS=0.001;

constexpr int INF = 2147483647;
constexpr float NEAR_Z=-1;
constexpr float FAR_Z=1;
constexpr float MAXFLOAT=std::numeric_limits<float>::max();
constexpr float MINFLOAT=std::numeric_limits<float>::min();
constexpr float PI=3.14159265358979323846;
constexpr float INV_PI=1.0/PI;
constexpr float OneMinusEpsilon = 0x1.fffffep-1;
constexpr float MAXPDFVALUE=100;

}