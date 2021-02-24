#include <stdlib.h>
#include <stdarg.h> 
#include <float.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>

//OUTPUT
#define OUTPUT_WIDTH 1920
#define OUTPUT_HEIGHT 1080
#define RAY_BOUNCES 8
#define RAYS_PER_PIXEL 8
#define MAX_RAYS_PER_PIXEL 4096

//PREPROCESSING
#define MIN_TRIANGLES_PER_LEAF 10
#define MIN_TRIANGLE_DIFFERENCE 3
#define PREPROCESSING_ONLY 0
#define SCENE_DRAGONS 1

//MULTITHREADING
#define NUMBER_OF_THREADS 8

//DEBUG
#define DEBUG_COLORS 0

#include "defines.h"


//Platform dependent code
#include "platform.cpp"
#include "threading.cpp"

//Utils
#include "math/math.cpp"
#include "math/math_vec.cpp"
#include "math/math_mat.cpp"
#include "math/math_quaternion.cpp"
#include "mesh.cpp"
#include "collada/collada.cpp"
#include "image.cpp"

//Ray tracing
#include "sampler.cpp"
#include "geometry.cpp"
#include "bounding_volumes.cpp"
#include "world.cpp"
#include "ray.cpp"
#include "tile_work.cpp"

struct command_line_options
{
    char* OutputFileName;
    u32 OutputHeight;
    u32 OutputWidth;
    u32 RaysPerPixel;
    u32 RayBounces;
    u32 NumberOfThreads;
    bool PreprocessingOnly;
};

internal command_line_options
ParseCommandLineOptions(int argc, char** argv)
{
    //Default values
    command_line_options Opt = {};
    Opt.OutputHeight = OUTPUT_HEIGHT;
    Opt.OutputWidth = OUTPUT_WIDTH;
    Opt.RaysPerPixel = RAYS_PER_PIXEL;
    Opt.RayBounces = RAY_BOUNCES;
    Opt.NumberOfThreads = NUMBER_OF_THREADS;
    Opt.PreprocessingOnly = PREPROCESSING_ONLY;
    Opt.OutputFileName = 0;
        
    char* UseHMessage = ", use -h for help\n";
    
    for(int i = 1; i < argc; i++)
    {
        char* arg = argv[i];
        
        if(arg[0] == '-')
        {
            switch(arg[1])
            {
                case 'o': {
                    if(argc - i <= 2) {
                        printf("Expected width and height for the output after -o%s", UseHMessage);
                        exit(1);
                    }
                    
                    Opt.OutputWidth = atoi(argv[++i]);
                    Opt.OutputHeight = atoi(argv[++i]);
                    
                    if(Opt.OutputWidth == 0 || Opt.OutputWidth > (1 << 16) ||
                       Opt.OutputHeight == 0 || Opt.OutputHeight > (1 << 16))
                    {
                        printf("Output resolution must be two integer values between one and 2^16");
                        exit(1);
                    }
                } break;
                
                case 'r': {
                    if(argc - i <= 1) {
                        printf("Expected number of rays per pixel after -r%s", UseHMessage);
                        exit(1);
                    }
                    
                    Opt.RaysPerPixel = atoi(argv[++i]);
                    if(Opt.RaysPerPixel == 0 || Opt.RaysPerPixel > MAX_RAYS_PER_PIXEL)
                    {
                        printf("Number of rays per pixel must be integer between one and 2^16");
                        exit(1);
                    }
                } break;
                
                case 'b': {
                    if(argc - i <= 1) {
                        printf("Expected number of rays bounces after -b%s", UseHMessage);
                        exit(1);
                    }
                    
                    Opt.RayBounces = atoi(argv[++i]);
                    if(Opt.RayBounces == 0 || Opt.RayBounces > (1 << 16))
                    {
                        printf("Number of ray bounces must be integer between one and 2^16");
                        exit(1);
                    }
                } break;
                
                case 'j': {
                    if(argc - i <= 1) {
                        printf("Expected number of threads -j%s", UseHMessage);
                        exit(1);
                    }
                    
                    Opt.NumberOfThreads = atoi(argv[++i]);
                    if(Opt.NumberOfThreads == 0 || Opt.NumberOfThreads > 512)
                    {
                        printf("Number of threads  must be integer between one and 512");
                        exit(1);
                    }
                } break;
                
                case 'p': {
                    Opt.PreprocessingOnly = true;
                } break;
                
                case 'h': {
                    printf("Usage: %s OUTPUT_FILE [OPTIONS]...\n", argv[0]);
                    printf("    -o WIDTH HEIGHT    specify output resolution\n");
                    printf("    -r RAYS            specify number of rays per pixel\n");
                    printf("    -b BOUNCES         specify number of bounces per ray\n");
                    printf("    -j THREADS         specify number of threads to use\n");
                    printf("    -p                 only do mesh preprocessing and print stats\n");
                    printf("    -h                 show this message\n");
                    exit(1);
                } break;
                
                default: {
                    printf("Invalid argument %s%s", arg, UseHMessage);
                    exit(1);
                } break;
            }
        }
        else
        {
            if(Opt.OutputFileName)
            {
                printf("Invalid argument %s%s", arg, UseHMessage);
                exit(1);
            }
            
            Opt.OutputFileName = arg;
        }
    }
    
    if(!Opt.OutputFileName)
    {
        printf("Must specify an output file path%s", UseHMessage);
        exit(1);
    }
    
    return Opt;
}

int main(int argc,char** argv)
{
    //Parse command line options
    command_line_options Opt = ParseCommandLineOptions(argc, argv);
    
    u32 OutputHeight = Opt.OutputHeight;
    u32 OutputWidth = Opt.OutputWidth;
    u32 RaysPerPixel = Opt.RaysPerPixel;
    u32 RayBounces = Opt.RayBounces;
    u32 NumberOfThreads = Opt.NumberOfThreads;
    bool PreprocessingOnly = Opt.PreprocessingOnly;
    
    
    //Prepare output image
    image_data OutputImage = AllocateImage(OutputWidth, OutputHeight);
    
    
    //Init scene
    world World = AllocWorld(vec3(0.7f, 0.9f, 1.0f));
        
    char* DragonPath = "../res/dragon.dae";
    collada_scene DragonScene = ReadColladaFromFile(DragonPath);
    if(DragonScene.MeshesCount == 0) {
        printf("Failed to load collada file at %s\n", DragonPath);
        exit(1);
    }
    mesh_data Dragon = DragonScene.Meshes[0];
    mat4 ModelMatrix = Mat4Rotate(90.0f, vec3(1.0f, 0.0f, 0.0f));
    //Transform to Z up
    TransformMeshVertices(&Dragon, ModelMatrix);
    
#if 0
    PushMaterial(&World, vec3(1.f, 1.0f, 1.0f), vec3(0.0f), 0.5f);
    PushMaterial(&World, vec3(0.0f), vec3(1.0f), 0.0f);
    PushMaterial(&World,  vec3(0.3f, 1.0f, 0.3f), vec3(0.0f), 0.1f);
    PushMaterial(&World,  vec3(0.3f, 0.3f, 1.0f), vec3(0.0f), 0.1f);
    PushMaterial(&World,  vec3(1.0f, 0.0f, 0.0f), vec3(0.0f), 0.1f);
    
    PushMaterial(&World, vec3(0.804f, 0.498f, 0.196f), vec3(0.0f), 0.7f);
    PushMaterial(&World, vec3(1.0f, 1.0f, 1.0f), vec3(0.0f), 1.0f);
    PushMaterial(&World, vec3(0.3f, 0.6f, 0.9f), vec3(0.0f), 0.8f);
    //PushMaterial(&World, vec3(1.0f, 0.843f, 0.0f), vec3(0.0f), 0.7f);
    
    //PushMaterial(&World, vec3(1.0f, 0.7f, 0.7f), vec3(0.0f), 1.5f, false);
    
    PushPlane(&World, vec3(0, 0, 1), 0.0f, 2);
    PushPlane(&World, vec3(0, 0, -1), -3.0f, 1);
    
    PushPlane(&World, vec3(0, -1, 0), -2.0f, 0);
    PushPlane(&World, vec3(1, 0, 0), -2.0f, 3);
    PushPlane(&World, vec3(-1, 0, 0), -2.0f, 4);
    
    PushMeshInfo(&World, &Dragon);
    
    PushMesh(&World, 0, vec3(-1.0f, -2.0f, 0.0f), Mat3Rotate(vec3(0.0f, 0.0f, 1.0f), 90.0f), vec3(0.15f), 5);
    PushMesh(&World, 0, vec3(0.0f,  -2.0f, 0.0f), Mat3Rotate(vec3(0.0f, 0.0f, 1.0f), 90.0f), vec3(0.15f), 6);
    PushMesh(&World, 0, vec3(1.0f,  -2.0f, 0.0f), Mat3Rotate(vec3(0.0f, 0.0f, 1.0f), 90.0f), vec3(0.15f), 7);
    
    
    //Init camera
    vec3 CameraP = vec3(0, -8, 1);
    vec3 CameraZ = Normalize(vec3(0, 0, 0.7f) - CameraP);
    vec3 CameraX = Normalize(Cross(vec3(0, 0, 1), CameraZ));
    vec3 CameraY = Normalize(Cross(CameraZ, CameraX));
#elif 0
    //Plane
    PushMaterial(&World, vec3(1.f, 1.0f, 1.0f), vec3(0.0f), 0.5f);
    //PushMaterial(&World, vec3(0.0f), vec3(1.0f), 0.0f);
    
    //Dragons
    PushMaterial(&World, vec3(0.3f, 0.6f, 0.9f), vec3(0.0f), 0.8f);
    PushMaterial(&World, vec3(1, 1, 1), vec3(0.0f), 1.0f);
    PushMaterial(&World, vec3(0.804f, 0.498f, 0.196f), vec3(0.0f), 0.9f);
//    PushMaterial(&World, vec3(1, 1, 1), vec3(0.0f), 1.1f, true);
    
    //Spheres
    u32 FirstSphereMaterial = World.MaterialsCount;
    PushMaterial(&World, vec3(1,1,1), vec3(0), 1.0f);
    PushMaterial(&World, vec3(1,0,0), vec3(0), 0.1f);
    PushMaterial(&World, vec3(0,1,0), vec3(0), 0.8f);
    PushMaterial(&World, vec3(0,0,1), vec3(0), 0.2f);
    
    PushMaterial(&World, vec3(0.3f,0.2f,0.6f), vec3(0), 0.2f);
    PushMaterial(&World, vec3(1.0f,0.2f,0.1f), vec3(0), 0.3f);
    PushMaterial(&World, vec3(1.0f,0.6f,0.1f), vec3(0), 0.8f);
    PushMaterial(&World, vec3(0.3f, 1, 0.8f), vec3(0), 0.6f);
    
    u32 SphereMaterialsCount = World.MaterialsCount - FirstSphereMaterial;
    
    random_series ColSeries = RandSeries(324634);
    random_series PosSeries = RandSeries(3634);
    random_series SizeSeries = RandSeries(33);
    
    u32 SpheresX = 7;
    u32 SpheresY = 7;
    For(y, SpheresY)
    {
        For(x, SpheresX)
        {
#if 0
            f32 H = RandRange(&ColSeries, 0.0f, 360.0f);
            f32 S = RandRange(&ColSeries, 0.5f, 1.0f);
            f32 V = RandRange(&ColSeries, 0.5f, 1.0f);
            vec3 Color = HSVToRGB(H, 1, 1);
            PushMaterial(&World, vec3(Color), vec3(0), Color.a);
#endif
            f32 Radius = RandRange(&SizeSeries, 0.3f, 1.0f);
            f32 Off = 0.5f;
            
            if(x == 2 && y == 2) {
                Radius *= 0.5f;
            }
            
            vec3 Position;
            Position.x = (f32)x / SpheresX * 20 - 8 + RandRange(&PosSeries, -Off, Off);
            Position.y = (f32)y / SpheresY * 20 - 8 + RandRange(&PosSeries, -Off, Off);
            Position.z = Radius;
            
            
            u32 Material = RandU32(&ColSeries) % SphereMaterialsCount + FirstSphereMaterial;
            
            PushSphere(&World, Position, Radius, Material);
        }
    }
    
    PushPlane(&World, vec3(0, 0, 1), 0.0f, 0);
    //PushSphere(&World, vec3(0, 0, 0), 1.0f, 1);
    
    PushMeshInfo(&World, &Dragon);
    
    f32 DragonScale = 0.3f;
    PushMesh(&World, 0, vec3(-3.0f, 0.0f, 0.0f), Mat3Rotate(vec3(0.0f, 0.0f, 1.0f), 90.0f), vec3(DragonScale), 1);
    PushMesh(&World, 0, vec3(0.0f,  0.0f, 0.0f), Mat3Rotate(vec3(0.0f, 0.0f, 1.0f), 90.0f), vec3(DragonScale), 2);
    PushMesh(&World, 0, vec3(3.0f,  0.0f, 0.0f), Mat3Rotate(vec3(0.0f, 0.0f, 1.0f), 90.0f), vec3(DragonScale), 3);
    
    
    //Init camera
    vec3 CameraP = vec3(-8, -8, 8);
    vec3 CameraZ = Normalize(vec3(0, 0, 0) - CameraP);
    
    
    vec3 CameraX = Normalize(Cross(vec3(0, 0, 1), CameraZ));
    vec3 CameraY = Normalize(Cross(CameraZ, CameraX));
#elif 1
    
    PushMaterial(&World, vec3(0.5f, 0.5f, 0.5f), vec3(0.0f), 0.1f);
    PushMaterial(&World, vec3(0.7f, 0.5f, 0.3f), vec3(0.0f), 0.3f);
    PushMaterial(&World, vec3(0.3f, 0.6f, 0.9f), vec3(0.0f), 0.8f);
    PushMaterial(&World, vec3(0.0f), vec3(0.9f, 0.0, 0.0f), 0.0f);
    
    //Dragons
    PushMaterial(&World, vec3(1, 1, 1), vec3(0.0f), 1.0f);
    PushMaterial(&World, vec3(0.3f, 0.6f, 0.9f), vec3(0.0f), 0.8f);
    PushMaterial(&World, vec3(0.804f, 0.498f, 0.196f), vec3(0.0f), 0.9f);
    
//    PushMaterial(&World, vec3(1.0f, 1.0f, 1.0f), vec3(0.0f), 1.0f, true);
//    PushMaterial(&World, vec3(1.0f, 0.7f, 0.7f), vec3(0.0f), 1.1f, false);
//    PushMaterial(&World, vec3(1.0f, 1.0f, 1.0f), vec3(0.0f), 1.0f, true);
    
    
    PushPlane(&World, vec3(0, 0, 1), 0.0f, 0);
//    PushSphere(&World, vec3(-2, 1, 1), 0.7f, 2);
//    PushSphere(&World, vec3(4, -0.3f, 0), 2.0f, 3);
    PushSphere(&World, vec3(5.5, 7, 2.5), 2.5f, 4);
    PushSphere(&World, vec3(-5.5, 7, 2.5), 2.5f, 4);
    
    PushMeshInfo(&World, &Dragon);
    //PushMesh(&World, 0, vec3(0.0f), Mat3Rotate(vec3(0.0f, 0.0f, 1.0f), 90.0f), vec3(0.3f), 4);
    f32 DragonBigScale = 0.3f;
    f32 DragonSmallScale = 0.20f;
    PushMesh(&World, 0, vec3(-2.0f, 0.0f, 0.0f), Mat3Rotate(vec3(0.0f, 0.0f, 1.0f), 90.0f), vec3(DragonSmallScale), 5);
    PushMesh(&World, 0, vec3(0.0f,  0.0f, 0.0f), Mat3Rotate(vec3(0.0f, 0.0f, 1.0f), 90.0f), vec3(DragonBigScale), 4);
    PushMesh(&World, 0, vec3(2.0f,  0.0f, 0.0f), Mat3Rotate(vec3(0.0f, 0.0f, 1.0f), 90.0f), vec3(DragonSmallScale), 6);
    
    
    //Spheres
    u32 FirstSphereMaterial = World.MaterialsCount;
    PushMaterial(&World, vec3(1,1,1), vec3(0), 1.0f);
    PushMaterial(&World, vec3(1,0,0), vec3(0), 0.1f);
    PushMaterial(&World, vec3(0,1,0), vec3(0), 0.8f);
    PushMaterial(&World, vec3(0,0,1), vec3(0), 0.2f);
    
    PushMaterial(&World, vec3(0.3f,0.2f,0.6f), vec3(0), 0.2f);
    PushMaterial(&World, vec3(1.0f,0.2f,0.1f), vec3(0), 0.3f);
    PushMaterial(&World, vec3(1.0f,0.6f,0.1f), vec3(0), 0.8f);
    PushMaterial(&World, vec3(0.3f, 1, 0.8f), vec3(0), 0.6f);
    
    u32 SphereMaterialsCount = World.MaterialsCount - FirstSphereMaterial;
    
    random_series ColSeries = RandSeries(324634);
    random_series PosSeries = RandSeries(3634);
    random_series SizeSeries = RandSeries(33);
    
    u32 SpheresX = 15;
    u32 SpheresY = 15;
    
    f32 Center = 15;
    f32 Range = 30;
    For(y, SpheresY)
    {
        For(x, SpheresX)
        {
            f32 Radius = RandRange(&SizeSeries, 0.2f, 0.3f);
            f32 Off = 0.5f;

            vec3 Position;
            Position.x = (f32)x / SpheresX * Range - Center + RandRange(&PosSeries, -Off, Off);
            Position.y = (f32)y / SpheresY * Range - Center + RandRange(&PosSeries, -Off, Off);
            Position.z = Radius;
            
            
            u32 Material = RandU32(&ColSeries) % SphereMaterialsCount + FirstSphereMaterial;
            
            PushSphere(&World, Position, Radius, Material);
        }
    }
    
    
    //Init camera
    vec3 CameraP = vec3(0, -10, 1);
    vec3 CameraZ = Normalize(vec3(0, 0, 1) - CameraP);
    vec3 CameraX = Normalize(Cross(vec3(0, 0, 1), CameraZ));
    vec3 CameraY = Normalize(Cross(CameraZ, CameraX));
    
#else
    
    PushMaterial(&World, vec3(0.5f, 0.5f, 0.5f), vec3(0.0f), 0.1f);
    PushMaterial(&World, vec3(0.7f, 0.5f, 0.3f), vec3(0.0f), 0.3f);
    PushMaterial(&World, vec3(0.3f, 0.6f, 0.9f), vec3(0.0f), 0.8f);
    PushMaterial(&World, vec3(0.0f), vec3(0.9f, 0.0, 0.0f), 0.0f);
    PushMaterial(&World, vec3(1.0f, 1.0f, 1.0f), vec3(0.0f), 1.0f, true);
    
    
    PushPlane(&World, vec3(0, 0, 1), 0.0f, 0);
    PushSphere(&World, vec3(-2, 1, 1), 0.7f, 2);
    PushSphere(&World, vec3(4, -0.3f, 0), 2.0f, 3);
    PushSphere(&World, vec3(3, 5, 2), 2.0f, 4);
    PushSphere(&World, vec3(-5, 6, 2), 2.0f, 4);
    
    PushMeshInfo(&World, &Dragon);
    PushMesh(&World, 0, vec3(0.0f), Mat3Rotate(vec3(0.0f, 0.0f, 1.0f), 90.0f), vec3(0.3f), 4);

    //Init camera
    vec3 CameraP = vec3(0, -10, 1);
    vec3 CameraZ = Normalize(vec3(0, 0, 1) - CameraP);
    vec3 CameraX = Normalize(Cross(vec3(0, 0, 1), CameraZ));
    vec3 CameraY = Normalize(Cross(CameraZ, CameraX));
    
#endif
    //Preprocess meshes
    PreprocessWorldMeshes(&World, PreprocessingOnly);
    if(PreprocessingOnly) {
        return 0;
    }
    
    //Compute tile ranges for workers
    tile_work_array WorkArray = {};
    
    u32 TilesX = 16;
    u32 TilesY = 16;
    u32 TilesToDo = TilesX * TilesY;
    u32 TileWidth = OutputWidth / TilesX;
    u32 TileHeight = OutputHeight / TilesY;
    u32 LastTileWidth = OutputWidth - TileWidth * (TilesX - 1);
    u32 LastTileHeight = OutputHeight - TileHeight * (TilesY - 1);
    
    WorkArray.TotalEntries = TilesToDo;
    WorkArray.Entries = (tile_work_entry*)ZeroAlloc(sizeof(tile_work_entry) * TilesToDo);
    
    For(y, TilesY)
    {
        u32 CurrHeight = y == TilesY - 1 ? LastTileHeight : TileHeight;
        For(x, TilesX)
        {
            u32 CurrWidth = x == TilesX - 1 ? LastTileWidth : TileWidth;
            u32 TileIndex = y * TilesY + x;
            WorkArray.Entries[TileIndex].x = x * TileWidth;
            WorkArray.Entries[TileIndex].y = y * TileHeight;
            WorkArray.Entries[TileIndex].CountX = CurrWidth;
            WorkArray.Entries[TileIndex].CountY = CurrHeight;
            WorkArray.Entries[TileIndex].RandomSeries.State = rand();
        }
    }
    
    // Data common to all workers
    tile_worker_thread_init Init = {};
    Init.WorkArray = &WorkArray;
    Init.OutputWidth = OutputWidth;
    Init.OutputHeight = OutputHeight;
    Init.OutputImage = &OutputImage;
    Init.RaysPerPixel = RaysPerPixel;
    Init.RayBounces = RayBounces;
    Init.World = &World;
    Init.CameraP = CameraP;
    Init.CameraX = CameraX;
    Init.CameraY = CameraY;
    Init.CameraZ = CameraZ;
    Init.MainThreadId = GetCurrentThreadId();
    
    GetSamplePositions(Init.Samples, RaysPerPixel);
    
    //Create workers
    timestamp BeginCounter = GetCurrentCounter();
    u32 SecondaryThreadsCount = NumberOfThreads - 1;
    For(i, SecondaryThreadsCount)
    {
        CreateWorkerThread(TileWorkerProc, &Init);
    }
    TileWorkerProc(&Init);
    
    //Wait for other workers to finish but keep printing stats
    while(WorkArray.EntriesDone < WorkArray.TotalEntries)
    {
        Sleep(1);
    }
    
    printf("\rRay casting progress: 100%%");

    timestamp EndCounter = GetCurrentCounter();
    f32 SecondsElapsed = GetSecondsElapsed(BeginCounter, EndCounter);
    
    //Print stats
    printf("\n");
    printf("%u - %u Output size\n", OutputWidth, OutputHeight);
    printf("%u Rays per pixel - %u Rays Bounces\n", RaysPerPixel, RayBounces);
    printf("%" PRIu64 "/%" PRIu64 "(%.3f %%) rays-triangle intersections passed\n", 
               Init.TriangleTestsPassed, Init.TriangleTestsTotal, 
               (f64)Init.TriangleTestsPassed / (f64)Init.TriangleTestsTotal);
    printf("Casted %" PRIu64 " rays in %.3f seconds(%.3f MRays/s)\n", 
               Init.RaysCasted, SecondsElapsed, Init.RaysCasted / (SecondsElapsed * (1000 *1000)));
    
    //Output result to file
    WriteImageToBMPFile(&OutputImage, Opt.OutputFileName);
    
    return 0;
}