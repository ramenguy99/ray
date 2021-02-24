struct material
{
    vec3 Albedo;
    vec3 Emit;
    
    bool Specular; //Otherwise refractive
    float Specularity;
    float OneOverRefractiveIndex;
    
    image_data* AlbedoTexture;
};

struct plane_entry
{
    plane Plane;
    u32 MaterialIndex;
};

struct sphere_entry
{
    sphere Sphere;
    u32 MaterialIndex;
};

struct mesh_info
{
    mesh_data Data;
    aabb_tree* AABBTree;
};

struct mesh_entry
{
    u32 MeshIndex;
    u32 MaterialIndex;

    vec3 Position;
    mat3 Rotation;
    vec3 Scale;
    float ScaleDet;    //Used to scale distances
    vec3 InvScale;
    float InvScaleDet; //Inverse of ScaleDet
};

#define MAX_SPHERES 1024
#define MAX_PLANES 1024
#define MAX_MATERIALS 64
#define MAX_MESHES 64
#define MAX_MESHES_INFO 16

struct world
{
    vec3 BackgroundColor;
    
    plane_entry* Planes;
    u32 PlanesCount;
    
    sphere_entry* Spheres;
    u32 SpheresCount;
    
    mesh_entry* Meshes;
    u32 MeshesCount;
    
    mesh_info* MeshesInfo;
    u32 MeshesInfoCount;
    
    material* Materials;
    u32 MaterialsCount;
};
