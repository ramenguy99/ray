#include "geometry.h"

internal f32
SphereArea(sphere S)
{
    f32 Result = 4.0f * PI *  S.Radius * S.Radius;
    return Result;
}

internal f32
SphereVolume(sphere S)
{
    f32 Result = (4.0f / 3.0f) * PI * S.Radius * S.Radius * S.Radius;
    return Result;
}

inline sphere
SphereCircumscribingAABB(aabb AABB)
{
    vec3 Half = (AABB.Max - AABB.Min) * 0.5f;
    vec3 Center = AABB.Min + Half;
    
    sphere Result;
    Result.Center = Center;
    Result.Radius = Length(Half);
    
    return Result;
}

inline b32
IsPointInAABB(vec3 P, aabb AABB)
{
    return
        P.x <= AABB.Max.x &&
        P.y <= AABB.Max.y &&
        P.z <= AABB.Max.z &&
        P.x >= AABB.Min.x &&
        P.y >= AABB.Min.y &&
        P.z >= AABB.Min.z;
}

internal aabb
ComputeAABB(vec3* Positions, u32 Count)
{
    aabb Result = {};
    Result.Min = vec3(FLT_MAX);
    Result.Max = vec3(-FLT_MAX);
    for(u32 Index = 0; Index < Count; Index++)
    {
        vec3 P = Positions[Index];
        if(P.x > Result.Max.x) Result.Max.x = P.x;
        if(P.y > Result.Max.y) Result.Max.y = P.y;
        if(P.z > Result.Max.z) Result.Max.z = P.z;
        
        if(P.x < Result.Min.x) Result.Min.x = P.x;
        if(P.y < Result.Min.y) Result.Min.y = P.y;
        if(P.z < Result.Min.z) Result.Min.z = P.z;
    }
    
    return Result;
}

internal aabb
ComputeAABBIndexed(vec3* Positions, u32* Indices, u32 IndicesCount)
{
    aabb Result = {};
    Result.Min = vec3(FLT_MAX);
    Result.Max = vec3(-FLT_MAX);
    For(i, IndicesCount)
    {
        vec3 P = Positions[Indices[i]];
        if(P.x > Result.Max.x) Result.Max.x = P.x;
        if(P.y > Result.Max.y) Result.Max.y = P.y;
        if(P.z > Result.Max.z) Result.Max.z = P.z;
        
        if(P.x < Result.Min.x) Result.Min.x = P.x;
        if(P.y < Result.Min.y) Result.Min.y = P.y;
        if(P.z < Result.Min.z) Result.Min.z = P.z;
    }
    
    return Result;
}

inline void
UpdateAABB(aabb* AABB, vec3 P)
{
    if(P.x > AABB->Max.x) AABB->Max.x = P.x;
    if(P.y > AABB->Max.y) AABB->Max.y = P.y;
    if(P.z > AABB->Max.z) AABB->Max.z = P.z;
    
    if(P.x < AABB->Min.x) AABB->Min.x = P.x;
    if(P.y < AABB->Min.y) AABB->Min.y = P.y;
    if(P.z < AABB->Min.z) AABB->Min.z = P.z;
}

inline f32
AABBVolume(aabb AABB)
{
    vec3 Diagonal = AABB.Max - AABB.Min;
    float Result = Diagonal.x * Diagonal.y * Diagonal.z;
    
    return Result;
}

inline f32
AABBArea(aabb AABB)
{
    vec3 d = AABB.Max - AABB.Min;
    return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
}
