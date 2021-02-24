#include "world.h"

internal world
AllocWorld(vec3 BackgroundColor)
{
    world World = {};
    
    World.BackgroundColor = BackgroundColor;
    World.Spheres = (sphere_entry*)ZeroAlloc(MAX_SPHERES * sizeof(sphere_entry));
    World.Planes = (plane_entry*)ZeroAlloc(MAX_PLANES * sizeof(plane_entry));
    World.Meshes = (mesh_entry*)ZeroAlloc(MAX_MESHES * sizeof(mesh_entry));
    World.MeshesInfo = (mesh_info*)ZeroAlloc(MAX_MESHES_INFO * sizeof(mesh_info));
    World.Materials = (material*)ZeroAlloc(MAX_MATERIALS * sizeof(material));
    
    return World;
}

//Transform a position to world space multiplying by the inverse model matrix of a mesh entry
internal vec3
WorldToLocalP(mesh_entry* M, vec3 P)
{
    return (Mat3Transpose(M->Rotation) * (P - M->Position)) * M->InvScale;
}

//Transform a normal to world space multiplying by the inverse model matrix of a mesh entry
//ignoring translation and normalizing at the end
internal vec3
WorldToLocalN(mesh_entry* M, vec3 N)
{
    return Normalize((Mat3Transpose(M->Rotation) * N) * M->InvScale);
}

//Transform a position to local space of a mesh
internal vec3
LocalToWorldP(mesh_entry* M, vec3 P)
{
    return M->Rotation * (P * M->Scale) + M->Position;
}

//Transform a normal to local space of a mesh
internal vec3
LocalToWorldN(mesh_entry* M, vec3 P)
{
    return Normalize(M->Rotation * (P * M->Scale));
}

internal void
PushPlane(world* World, vec3 n, float d, u32 MaterialIndex)
{
    Assert(World->PlanesCount < MAX_PLANES);
    plane_entry* Entry = &World->Planes[World->PlanesCount++];
    Entry->Plane.Normal = n;
    Entry->Plane.d = d;
    Entry->MaterialIndex = MaterialIndex;
}


internal void
PushSphere(world* World, vec3 c, float r, u32 MaterialIndex)
{
    Assert(World->SpheresCount < MAX_SPHERES);
    sphere_entry* Entry = &World->Spheres[World->SpheresCount++];
    Entry->Sphere.Center = c;
    Entry->Sphere.Radius = r;
    Entry->MaterialIndex = MaterialIndex;
}

//Push an instance of a mesh
internal void
PushMesh(world* World, u32 MeshIndex, vec3 Position, mat3 Rotation, vec3 Scale, u32 MaterialIndex)
{
    Assert(World->MeshesCount < MAX_MESHES);
    mesh_entry* Entry = &World->Meshes[World->MeshesCount++];
    
    Entry->MeshIndex = MeshIndex;
    Entry->MaterialIndex = MaterialIndex;
    
    Entry->Position = Position;
    Entry->Rotation = Rotation;
    Entry->Scale = Scale;
    Entry->ScaleDet = powf(Scale.x * Scale.y * Scale.z, 1.0f / 3.0f);
    Entry->InvScale = 1.0f / Scale;
    Entry->InvScaleDet = 1.0f / Entry->ScaleDet;
}

//Push mesh data, can be used by multiple instances
internal void
PushMeshInfo(world* World, mesh_data* Data)
{
    Assert(World->MeshesInfoCount < MAX_MESHES_INFO);
    mesh_info* Info = &World->MeshesInfo[World->MeshesInfoCount++];
    Info->Data = *Data;
}

internal void
PushMaterial(world* World, vec3 Albedo, vec3 Emit, float Value, bool Specular = true)
{
    Assert(World->MaterialsCount < MAX_MATERIALS);
    material* Material = &World->Materials[World->MaterialsCount++];
    Material->Albedo = Albedo;
    Material->Emit = Emit;
    Material->Specular = Specular;
    if(Specular)
        Material->Specularity = Value;
    else
        Material->OneOverRefractiveIndex = 1.0f / Value;
}

internal void
PushTexturedMaterial(world* World, image_data* AlbedoTexture)
{
    Assert(World->MaterialsCount < MAX_MATERIALS);
    material* Material = &World->Materials[World->MaterialsCount++];
    Material->AlbedoTexture = AlbedoTexture;
}

//Compute aabb trees for each mesh_info, if Verbose print stats for each tree
internal void
PreprocessWorldMeshes(world* World, bool Verbose)
{
    if(Verbose)
        printf("AABB Preprocessing settings:\n %2u MIN_TRIANGLES_PER_LEAF\n %2u MIN_TRIANGLE_DIFFERENCE\n\n", MIN_TRIANGLES_PER_LEAF, MIN_TRIANGLE_DIFFERENCE);
    
    For(Index, World->MeshesInfoCount)
    {
        mesh_info* Mesh = &World->MeshesInfo[Index];
        timestamp Begin = GetCurrentCounter();
        
        Mesh->AABBTree = ComputeAABBTree(Mesh->Data.Positions, Mesh->Data.Indices, Mesh->Data.IndicesCount);
        timestamp End = GetCurrentCounter();
        f32 SecondsElapsed = GetSecondsElapsed(Begin, End);
        
        if(Verbose)
        {
            printf("Mesh %u: %u triangles (%.3f ms):\n", Index, Mesh->Data.IndicesCount / 3, SecondsElapsed * 1000.0f);
            PrintAABBInfo(Mesh->AABBTree);
        }
    }
}
