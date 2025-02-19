#pragma once
#include<iostream>

enum class InterpolateSignal{
    Normal          =1<<0,
    FragPos_World   =1<<1,
    UV              =1<<2,
    Color           =1<<3,

    ALL_INP = Normal|FragPos_World|UV|Color,
};
inline InterpolateSignal operator|(const InterpolateSignal& s1,const InterpolateSignal& s2){
    return (InterpolateSignal)((int)(s1)|(int)(s2));
}
inline InterpolateSignal operator&(const InterpolateSignal& s1,const InterpolateSignal& s2){
    return (InterpolateSignal)((int)(s1)&(int)(s2));
}
inline InterpolateSignal operator^(const InterpolateSignal& s1,const InterpolateSignal& s2){
    return (InterpolateSignal)((int)(s1)^(int)(s2));
}

// specify object's shader type
enum class ShaderType{
    // decide how to fetch color
    Texture       =1<<0,
    Color         =1<<1,         
    Depth         =1<<2,
    Normal        =1<<3,
    Frame         =1<<4,
    
    // decide shader's type
    BlinnPhone    =1<<5,
    Light         =1<<6,

    ORDER=Texture|Color|Normal|Frame,
    LIGHTSHADER=BlinnPhone,

};
inline ShaderType operator|(const ShaderType& s1,const ShaderType& s2){
    return (ShaderType)((int)(s1)|(int)(s2));
}
inline ShaderType operator&(const ShaderType& s1,const ShaderType& s2){
    return (ShaderType)((int)(s1)&(int)(s2));
}
inline ShaderType operator^(const ShaderType& s1,const ShaderType& s2){
    return (ShaderType)((int)(s1)^(int)(s2));
}

enum class ClipFlag{
    accecpted=0,
    clipped=1,
    refused=2,
};

enum class ClipPlane {
    Left,
    Right,
    Bottom,
    Top,
    Near,
    Far
};

enum class RasterizeType{
    Naive       =1<<0,
    Bvh_hzb     =1<<1,
    Easy_hzb    =1<<2,
    Scan_convert=1<<3,
};

