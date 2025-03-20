#pragma once
#include <iostream>
#include <chrono>
#include <map>
#include <string>
#include <mutex>
#include<stdexcept>

enum class TimerType
{
    VertexProcessing,
    pipelineClipping,
    Rasterization,
    vertexShader,
    FrameProcessing,
};

class CPUTimer
{
public:
    CPUTimer() {}
    ~CPUTimer() {}

    // 启动指定类型的计时器
    void start(std::string type)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto &timer = timers_[type];
        if (!timer.running)
        {
            timer.start_time = std::chrono::high_resolution_clock::now();
            timer.running = true; // 防止重复调用喔
        }
    }

    // 停止指定类型的计时器并累积时间
    void stop(std::string type)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = timers_.find(type);
        if (it != timers_.end() && it->second.running)
        {
            auto end_time = std::chrono::high_resolution_clock::now();
            it->second.elapsed_time += std::chrono::duration_cast<std::chrono::microseconds>(end_time - it->second.start_time).count();
            it->second.running = false;
        }
    }
    // 丢掉某一个计时器
    void del(std::string type){
        std::lock_guard<std::mutex> lock(mutex_);
        timers_.erase(type);
    }

    // 重置所有计时器
    void reset()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto &[type, timer] : timers_)
        {
            timer.elapsed_time = 0.0;
            timer.running = false;
        }
    }

    // 输出所有计时器的累积时间
    void report()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout <<"\n ------------------------------- "<<std::endl;
        for (const auto &[type, timer] : timers_)
        {
            std::cout << type << " : " << (timer.elapsed_time) / 1000.0 << " ms" << std::endl;
        }
        std::cout <<" ------------------------------- "<<std::endl;
    }

    // 获取指定类型计时器的累积时间(s)
    float getElapsedTime(std::string type)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = timers_.find(type);
        if (it != timers_.end())
            return it->second.elapsed_time / 1000000.0;
        else{
            throw std::runtime_error("getElapsedTime::no such timer...");
        }
    }

    void clear(){
        timers_.clear();
    }

public:
    struct Timer
    {
        double elapsed_time = 0.0; // 累积时间（微秒）
        bool running = false;
        std::chrono::high_resolution_clock::time_point start_time;
    };

    std::map<std::string, Timer> timers_;
    std::mutex mutex_; // thread safty
};