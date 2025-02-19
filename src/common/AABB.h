#pragma once
#include<iostream>
#include"common_include.h"
#include"algorithm"

struct AABB2d{
    glm::vec2 min;
    glm::vec2 max;
    bool valid=true;
    AABB2d():min(srender::INF,srender::INF),max(-srender::INF,-srender::INF){}

    // contain a line in a conservative way: represent point in discrete integers(corresponding to pixels in screen)
    void containLine(const glm::vec2& p1,const glm::vec2& p2){
        min.x=(int)std::min(p1.x,p2.x)-1;
        min.y=(int)std::min(p1.y,p2.y)-1;
        max.x=(int)std::max(p1.x,p2.x)+1;
        max.y=(int)std::max(p1.y,p2.y)+1;
    }
    
    
    void containTriangel(const glm::vec2& p1,const glm::vec2& p2,const glm::vec2& p3){
        min.x=std::min(std::min(p1.x,p2.x),p3.x);
        min.y=std::min(std::min(p1.y,p2.y),p3.y);
        max.x=std::max(std::max(p1.x,p2.x),p3.x);
        max.y=std::max(std::max(p1.y,p2.y),p3.y);
        if(min.x>=max.x||min.y>=max.y)
            valid=false;
    }

    void clipAABB(const AABB2d& box){
        max.y=std::min(max.y,box.max.y);
        max.x=std::min(max.x,box.max.x);

        min.y=std::max(min.y,box.min.y);
        min.x=std::max(min.x,box.min.x);

        if(min.x>=max.x||min.y>=max.y)
            valid=false;
    }
};

struct AABB3d{
    glm::vec3 min;
    glm::vec3 max;
    AABB3d():min(srender::INF,srender::INF,srender::INF),max(-srender::INF,-srender::INF,-srender::INF){}
    AABB3d(const glm::vec3& p1,const glm::vec3& p2,const glm::vec3& p3){
        min.x=std::min(std::min(p1.x,p2.x),p3.x);
        min.y=std::min(std::min(p1.y,p2.y),p3.y);
        min.z=std::min(std::min(p1.z,p2.z),p3.z);
        max.x=std::max(std::max(p1.x,p2.x),p3.x);
        max.y=std::max(std::max(p1.y,p2.y),p3.y);
        max.z=std::max(std::max(p1.z,p2.z),p3.z);

    }

    AABB3d& operator=(const AABB3d& box){
        if(this==&box) return *this;
        
        min=box.min;
        max=box.max;
        return *this;
    }


    void reset(){
        min={srender::INF,srender::INF,srender::INF};
        max={-srender::INF,-srender::INF,-srender::INF};
    }

    void addPoint(const glm::vec3& point){
        min.x=std::min(min.x,point.x);
        min.y=std::min(min.y,point.y);
        min.z=std::min(min.z,point.z);
        
        max.x=std::max(max.x,point.x);
        max.y=std::max(max.y,point.y);
        max.z=std::max(max.z,point.z);
    }

    void expand(const AABB3d& box){
        min.x=std::min(min.x,box.min.x);
        min.y=std::min(min.y,box.min.y);
        min.z=std::min(min.z,box.min.z);
        
        max.x=std::max(max.x,box.max.x);
        max.y=std::max(max.y,box.max.y);
        max.z=std::max(max.z,box.max.z);
    }

    void clipAABB(const AABB3d& box){
        min.x=std::max(min.x,box.min.x);
        min.y=std::max(min.y,box.min.y);
        min.z=std::max(min.z,box.min.z);
        
        max.x=std::min(max.x,box.max.x);
        max.y=std::min(max.y,box.max.y);
        max.z=std::min(max.z,box.max.z);
    }

    float extent(int axis) const{
        switch(axis){
            case 0: return max.x-min.x;
            case 1: return max.y-min.y;
            case 2: return max.z-min.z;
        }
        return 0;
    }

    AABB3d transform(const glm::mat4& m){
        std::vector<glm::vec3> bboxpoints={min,{min.x,min.y,max.z},{min.x,max.y,max.z},{min.x,max.y,min.z},
                           {max.x,min.y,min.z},{max.x,min.y,max.z},                 max,{max.x,max.y,min.z}};
        min=glm::vec3(srender::INF,srender::INF,srender::INF);
        max=glm::vec3(-srender::INF,-srender::INF,-srender::INF);
        for(auto& p:bboxpoints){
            p=m*glm::vec4(p,1);
            min.x=std::min(min.x,p.x);
            min.y=std::min(min.y,p.y);
            min.z=std::min(min.z,p.z);
            
            max.x=std::max(max.x,p.x);
            max.y=std::max(max.y,p.y);
            max.z=std::max(max.z,p.z);
        }
        return *this;
    }
    
};
