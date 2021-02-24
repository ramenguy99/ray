#include "ray.h"

//Returns distance from ray origin to intersection point
//Positive if forward, negative if backwards
inline f32
RayPlaneIntersect(plane Plane, vec3 p, vec3 dir)
{
    vec3 n = Plane.Normal;
    float d = Plane.d;
    
    float Denominator = Dot(n, dir);
    if(fabs(Denominator) < 0.001f)
    {
        return FLT_MAX;
    }

    float t = (d - Dot(n, p)) / Denominator;
    
    return t;
}

//Return true if there is an intersection
inline b32
RaySphereTest(sphere Sphere, vec3 p, vec3 dir)
{
    vec3 m = p - Sphere.Center;
    float c = Dot(m, m) - Sphere.Radius * Sphere.Radius;
    if(c <= 0.0f) return true;
    
    //Ray Pointing away from sphere
    float b = Dot(m, dir);
    if (b > 0.0f) return false;
    
    //Ray missed sphere
    float discr = b*b - c;
    if(discr < 0.0f) return false;
    
    return true;
}

//Return distance along ray at which we intersect, FLT_MAX if miss
inline f32
RaySphereIntersect(sphere Sphere, vec3 p, vec3 dir)
{
    vec3 m = p - Sphere.Center;
    float b = Dot(m, dir);
    float c = Dot(m, m) - Sphere.Radius * Sphere.Radius;
    
    //Ray Pointing away from sphere
    if (c > 0.0f && b > 0.0f) return FLT_MAX;
    
    //Ray missed sphere
    float discr = b*b - c;
    if(discr < 0.0f) return FLT_MAX;
    
    float t = -b - (f32)sqrt(discr);
    //Ray inside sphere
    if(t < 0.0f) t = 0.0f;
    
    return t;
}

//Thread local variables for stats
thread_local u64 Thread_TriangleTestsPassed = 0;
thread_local u64 Thread_TriangleTestsTotal = 0;

// Given ray from origin p and direction dir and triangle abc,
// returns distance of intersection between ray and
// triangle and also returns the barycentric coordinates (u,v,w)
// of the intersection point
inline f32
RayTriangleIntersect(vec3 a, vec3 b, vec3 c, vec3 p, vec3 dir, vec3* UVW)
{
    Thread_TriangleTestsTotal++;
    
    vec3 ab = b - a;
    vec3 ac = c - a;
    vec3 qp = Negate(dir);
    // Compute triangle normal. Can be precalculated or cached if
    // intersecting multiple segments against the same triangle
    vec3 n = Cross(ac, ab); //reverse
    // Compute denominator d. If d <= 0, segment is parallel to or points
    // away from triangle, so exit early
    float d = Dot(qp, n);
    if (d <= 0.0f) return FLT_MAX;
    // Compute intersection t value of pq with plane of triangle. A ray
    // intersects iff 0 <= t. Segment intersects iff 0 <= t <= 1. Delay
    // dividing by d until intersection has been found to pierce triangle
    vec3 ap = p - a;
    f32 t = Dot(ap, n);
    if (t < 0.0f) return FLT_MAX;
    // Compute barycentric coordinate components and test if within bounds
    vec3 e = Cross(ap, qp); //reverse
    f32 v = Dot(ac, e);
    if (v < 0.0f || v > d) return FLT_MAX;
    f32 w = -Dot(ab, e);
    if (w < 0.0f || v + w > d) return FLT_MAX;
    // Segment/ray intersects triangle. Perform delayed division and
    // compute the last barycentric coordinate component
    f32 ood = 1.0f / d;
    t *= ood;
    v *= ood;
    w *= ood;
    f32 u = 1.0f - v - w;
    *UVW = vec3(u, v, w);
    
    Thread_TriangleTestsPassed++;
    return t;
}

inline vec3
UVWInterpolate(vec3 a, vec3 b, vec3 c, vec3 UVW)
{
    vec3 Result = a * UVW.x + b * UVW.y + c * UVW.z;
    return Result;
}

inline vec2
UVWInterpolate(vec2 a, vec2 b, vec2 c, vec3 UVW)
{
    vec2 Result = a * UVW.x + b * UVW.y + c * UVW.z;
    return Result;
}

//Return distance along ray at which we intersect, FLT_MAX if miss
internal f32
RayAABBTest(aabb a, vec3 p, vec3 dir)
{
    float tmin = 0; // set to -FLT_MAX to get first hit on line
    float tmax = FLT_MAX; // set to max distance ray can travel (for segment
    
    // For all three slabs
    for (int i = 0; i < 3; i++) {
        if (fabs(dir.e[i]) < 0.000001f) {
            // Ray is parallel to slab. No hit if origin not within slab
            if (p.e[i] < a.Min.e[i] || p.e[i] > a.Max.e[i]) return FLT_MAX;
        } else {
            // Compute intersection t value of ray with near and far plane of slab
            float ood = 1.0f / dir.e[i];
            float t1 = (a.Min.e[i] - p.e[i]) * ood;
            float t2 = (a.Max.e[i] - p.e[i]) * ood;
            // Make t1 be intersection with near plane, t2 with far plane
            if (t1 > t2) {
                f32 temp = t1;
                t1 = t2;
                t2 = temp;
            }
            // Compute the intersection of slab intersection intervals
            if (t1 > tmin) tmin = t1;
            if (t2 < tmax) tmax = t2;
            // Exit with no collision as soon as slab intersection becomes empty
            if (tmin > tmax) return FLT_MAX;
        }
    }
    // Ray intersects all 3 slabs. q is point and tmin distance
    //    q = p + dir * tmin;
    return tmin;
}

inline b32
PointAABBTest(vec3 Point, aabb AABB)
{
    return AABB.Min.x <= Point.x &&  Point.x <= AABB.Max.x &&
           AABB.Min.y <= Point.y &&  Point.y <= AABB.Max.y &&
           AABB.Min.z <= Point.z &&  Point.z <= AABB.Max.z;
}

//Intersect ray with list of indexed triangles
inline ray_triangle_intersection
RayIndexedTrianglesIntersect(u32* Indices, u32 IndicesCount, vec3 p, vec3 dir, vec3* Positions, vec3* Normals, vec2* UVs)
{
    f32 MinDistance = FLT_MAX;
    vec3 MinUVW;
    u32 Mini0 = 0;
    u32 Mini1 = 0;
    u32 Mini2 = 0;
    
    Assert(IndicesCount % 3 == 0);
    for(u32 i = 0; i < IndicesCount; i += 3)
    {
        u32 i0 = Indices[i + 0];
        u32 i1 = Indices[i + 2];
        u32 i2 = Indices[i + 1];
        
        vec3 a = Positions[i0];
        vec3 b = Positions[i1];
        vec3 c = Positions[i2];
        
        vec3 UVW = vec3(0.0f);
        f32 Distance = RayTriangleIntersect(a, b, c, p, dir, &UVW);
        if(Distance > 0.0f && Distance < MinDistance)
        {
            MinDistance = Distance;
            MinUVW = UVW;
            Mini0 = i0;
            Mini1 = i1;
            Mini2 = i2;
        }
    }
    
    ray_triangle_intersection Result = {};
    Result.Distance = MinDistance;
    Result.UVW = MinUVW;
    Result.i0 = Mini0;
    Result.i1 = Mini1;
    Result.i2 = Mini2;
    return Result;
}

//Intersect ray with aabbtree recursively
internal ray_triangle_intersection
RayMeshAABBTreeIntersectRec(aabb_tree* Tree, vec3 p, vec3 dir, float Distance, vec3* Positions, vec3* Normals, vec2* UVs)
{
    //If leaf intersect with all the contained triangles
    if(!Tree->Left && !Tree->Right)
    {
        return RayIndexedTrianglesIntersect(Tree->Indices, Tree->IndicesCount, p, dir,
                                            Positions, Normals, UVs);
    }
    
    //Intersect ray with children aabb
    ray_triangle_intersection ResultLeft, ResultRight;
    f32 LeftDistance = Tree->Left ? RayAABBTest(Tree->Left->AABB, p, dir) : FLT_MAX;
    f32 RightDistance = Tree->Right ? RayAABBTest(Tree->Right->AABB, p, dir) : FLT_MAX;
    
    b32 TraverseLeft = LeftDistance < Distance;
    b32 TraverseRight = RightDistance < Distance;
    
    //If we have both children to traverse we traverse the closest first
    if(TraverseLeft && TraverseRight)
    {
        if(LeftDistance < RightDistance)
        {
            ResultLeft = RayMeshAABBTreeIntersectRec(Tree->Left, p, dir, Distance, Positions, Normals, UVs);
            //We update our current distance if lower and traverse the other only if in range
            if(ResultLeft.Distance < Distance) Distance = ResultLeft.Distance;
            if(RightDistance < Distance)
            {
                ResultRight = RayMeshAABBTreeIntersectRec(Tree->Right, p, dir, Distance, Positions, Normals, UVs);
                //finally return the better result
                return ResultLeft.Distance < ResultRight.Distance ? ResultLeft : ResultRight;
            }
            else
            {
                return ResultLeft;
            }
        }
        else
        {
            ResultRight = RayMeshAABBTreeIntersectRec(Tree->Right, p, dir, Distance, Positions, Normals, UVs);
            if(ResultRight.Distance < Distance) Distance = ResultRight.Distance;
            if(LeftDistance < Distance)
            {
                ResultLeft = RayMeshAABBTreeIntersectRec(Tree->Left, p, dir, Distance, Positions, Normals, UVs);
                return ResultLeft.Distance < ResultRight.Distance ? ResultLeft : ResultRight;
            }
            else
            {
                return ResultRight;
            }
        }
    }
    else
    {
        //If we only have 1 child traversable
        if(TraverseLeft)
        {
            return RayMeshAABBTreeIntersectRec(Tree->Left, p, dir, Distance, Positions, Normals, UVs);
        }
        
        if(TraverseRight)
        {
            return RayMeshAABBTreeIntersectRec(Tree->Right, p, dir, Distance, Positions, Normals, UVs);
        }
        
        //If we have no child traversable
        ray_triangle_intersection Result = {};
        Result.Distance = FLT_MAX;
        return Result;
    }
}

//Intersect ray with aabbtree and compute normals and uvs at hit point
inline f32
RayMeshAABBTreeIntersect(mesh_info* Mesh, vec3 p, vec3 dir, f32 Distance, vec3* HitNormal, vec2* HitUV)
{
    vec3* Positions = Mesh->Data.Positions;
    vec3* Normals = Mesh->Data.Normals;
    vec2* UVs = Mesh->Data.UVs;
    
    ray_triangle_intersection Result = {};
    
    //First test with the root aabb
    f32 RootDistance = RayAABBTest(Mesh->AABBTree->AABB, p, dir);
    if(RootDistance < Distance)
    {
        Result = RayMeshAABBTreeIntersectRec(Mesh->AABBTree, p, dir, Distance, Positions, Normals, UVs);
    }
    else
    {
        Result.Distance = FLT_MAX;
    }
    
    //If we have a hit compute normals and uvs by interpolating vertex values
    if(Result.Distance > 0.0f && Result.Distance < FLT_MAX)
    {
        vec3 n0 = Normals[Result.i0];
        vec3 n1 = Normals[Result.i1];
        vec3 n2 = Normals[Result.i2];
        vec3 Normal = UVWInterpolate(n0, n1, n2, Result.UVW);
        *HitNormal = Normalize(Normal);
        
        vec2 t0 = UVs[Result.i0];
        vec2 t1 = UVs[Result.i1];
        vec2 t2 = UVs[Result.i2];
        vec2 UV = UVWInterpolate(t0, t1, t2, Result.UVW);
        *HitUV = UV;
    }
    
    return Result.Distance;
}

internal vec4
BilinearSampleImage(image_data* Image, vec2 UV)
{
    f32 u = UV.x;
    f32 v = UV.y;
    
    u = u * Image->Width - 0.5f;
    v = v * Image->Height - 0.5f;
    s32 x = (s32)u;
    s32 y = (s32)v;
    f32 u_ratio = u - x;
    f32 v_ratio = v - y;
    f32 u_opposite = 1 - u_ratio;
    f32 v_opposite = 1 - v_ratio;
    
    u32 P00 = GetRGBAPixel(Image, x, y);
    u32 P10 = GetRGBAPixel(Image, x + 1, y);
    u32 P01 = GetRGBAPixel(Image, x, y + 1);
    u32 P11 = GetRGBAPixel(Image, x + 1, y + 1);
    
    vec4 T00 = SRGBToLinear(RGBAToVec4(P00));
    vec4 T10 = SRGBToLinear(RGBAToVec4(P10));
    vec4 T01 = SRGBToLinear(RGBAToVec4(P01));
    vec4 T11 = SRGBToLinear(RGBAToVec4(P11));
    
    vec4 Result = (T00 * u_opposite  + T10 * u_ratio) * v_opposite +
                  (T01 * u_opposite  + T11 * u_ratio) * v_ratio;
    
    return Result;
}

//
internal vec3
RayCast(world* World, vec3 Origin, vec3 Direction, u32 Bounces, random_series* Series)
{
    vec3 Result = vec3(0.0f);
    
    vec3 DebugColor = vec3(0.0f);
#if DEBUG_COLORS
    u32 RayBounceCount = 1;
#else
    u32 RayBounceCount = Bounces;
#endif
    
    vec3 Attenuation = vec3(1.0f);
    
    // Keep going until we hit the max number of bounces
    For(BounceIndex, RayBounceCount)
    {
        f32 HitDistance = FLT_MAX;
        
        u32 HitMaterialIndex = (u32)-1;
        vec3 HitNormal;
        vec2 HitUV = vec2(0.0f);
        
        //Intersect all planes
        For(Index, World->PlanesCount)
        {
            plane_entry* Entry = &World->Planes[Index];
            f32 Distance = RayPlaneIntersect(Entry->Plane, Origin, Direction);

            if(Distance > 0.0f && Distance < HitDistance)
            {
                HitDistance = Distance;
                HitMaterialIndex = Entry->MaterialIndex;
                HitNormal = Entry->Plane.Normal;
                
                DebugColor = (HitNormal + 1.0f) * 0.5f;
            }
        }
        
        //Intersect all spheres
        For(Index, World->SpheresCount)
        {
            sphere_entry* Entry = &World->Spheres[Index];
            f32 Distance = RaySphereIntersect(Entry->Sphere, Origin, Direction);
            if(Distance > 0.0f && Distance < HitDistance)
            {
                HitDistance = Distance;
                HitMaterialIndex = Entry->MaterialIndex;
                vec3 Point = Distance * Direction + Origin;
                HitNormal = Normalize(Point - Entry->Sphere.Center);
                
                DebugColor = (HitNormal + 1.0f) * 0.5f;
            }
        }
        
        //Intersect all meshes
        For(Index, World->MeshesCount)
        {
            mesh_entry* Entry = &World->Meshes[Index];
            mesh_info* Mesh = &World->MeshesInfo[Entry->MeshIndex];
            
            vec3 lOrigin = WorldToLocalP(Entry, Origin);
            vec3 lDirection = WorldToLocalN(Entry, Direction);
            f32 lHitDistance = HitDistance * Entry->InvScaleDet; 
            if(HitDistance == FLT_MAX)
            {
                lHitDistance = FLT_MAX;
            }
            
            vec3 lNormal;
            vec2 UV;
            
            f32 lDistance = RayMeshAABBTreeIntersect(Mesh, lOrigin, lDirection, lHitDistance, &lNormal, &UV);
            
            if(lDistance > 0.0f && lDistance < lHitDistance)
            {
                HitDistance = lDistance * Entry->ScaleDet;
                HitNormal = LocalToWorldN(Entry, lNormal);
                HitUV = UV;
                HitMaterialIndex = Entry->MaterialIndex;
                
                DebugColor = (HitNormal + 1.0f) * 0.5f;
            }
        }
        
        //Compute color and next ray direction if we have a hit
        if(HitMaterialIndex != -1){
            Assert(HitMaterialIndex < World->MaterialsCount);
            material* Material = &World->Materials[HitMaterialIndex];
            
#if DEBUG_COLORS
            Result = DebugColor;
#else
            Result = Result + Attenuation * Material->Emit;
#endif
            vec3 Albedo = Material->Albedo;
            if(Material->AlbedoTexture)
            {
                Albedo = vec3(BilinearSampleImage(Material->AlbedoTexture, HitUV));
            }
            
            
            Origin = Origin + Direction * HitDistance;
            
            //Specular material
            if(Material->Specular)
            {
                
                f32 CosineFactor = Dot(-Direction, HitNormal);
                if(CosineFactor < 0.0f) CosineFactor = 0.0f;
                Attenuation = Attenuation * Albedo * CosineFactor;
                
                //Add a small tolerance value along normal to surface
                Origin = Origin + HitNormal * 0.000001f;
                vec3 PureBounce = Bounce(Direction, HitNormal);
                vec3 RandBounce = Normalize(HitNormal + RandDir(Series));
                // vec3 RandBounce = Normalize(HitNormal + vec3(RandNO(Series), RandNO(Series), RandNO(Series)));
                
                Direction = Normalize(Lerp(RandBounce, PureBounce, Material->Specularity));
            }
            //Refractive material
            else
            {
                
                f32 CosineFactor = Dot(-Direction, HitNormal);
                if(CosineFactor < 0.0f) CosineFactor = 0.0f;
                Attenuation = Attenuation * Albedo * CosineFactor;
                
                //Add a small tolerance value along ray direction
                Origin = Origin + Direction * 0.0001f;
                vec3 Refr = Refract(Direction, HitNormal, Material->OneOverRefractiveIndex);
                Direction = Normalize(Refr);
            }
        } else {
            //Missed everything, add backgroung color and break
            Result = Result + Attenuation * World->BackgroundColor;
            break;
        }
    }
    
    return Result;
}


