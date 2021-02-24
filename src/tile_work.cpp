#include "tile_work.h"

THREAD_PROC(TileWorkerProc)
{
    // Extract work info into locals
    tile_worker_thread_init* Init = (tile_worker_thread_init*)Data;
    
    u32 OutputWidth = Init->OutputWidth;
    u32 OutputHeight = Init->OutputHeight;
    u32 RaysPerPixel = Init->RaysPerPixel;
    u32 Bounces = Init->RayBounces;
    s64 TotalRaysToCast = RaysPerPixel * OutputWidth * OutputHeight;
    vec2* Samples = Init->Samples;
    
    world* World = Init->World;
    vec3 CameraP = Init->CameraP;
    vec3 CameraX = Init->CameraX;
    vec3 CameraY = Init->CameraY;
    vec3 CameraZ = Init->CameraZ;
    
    image_data* OutputImage = Init->OutputImage;
    
    thread_id MainThreadId = Init->MainThreadId;
    thread_id ThreadId = GetCurrentThreadId();
    u32 PercentageCounter = 0;
    
    //Compute film parameters
    f32 FilmDist = 1.0f;
    f32 FilmW = 1.0f;
    f32 FilmH = 1.0f;
    
    if(OutputWidth > OutputHeight)
    {
        FilmH = (f32)OutputHeight / (f32)OutputWidth;
    }
    else
    {
        FilmW = (f32)OutputWidth / (f32)OutputHeight;
    }
    
    f32 HalfFilmW = FilmW * 0.5f;
    f32 HalfFilmH = FilmH * 0.5f;
    vec3 FilmCenter = CameraP - FilmDist * CameraZ;
    f32 HalfPixW = 1.0f / OutputWidth;
    f32 HalfPixH = 1.0f / OutputHeight;
    
    f32 PixW = 2.0f / OutputWidth;
    f32 PixH = 2.0f / OutputHeight;
    
    //Work loop
    tile_work_array* WorkArray = Init->WorkArray;
    while(true)
    {
        //Get and increment work index
        u32 WorkIndex = InterlockedIncrement(&WorkArray->CurrentEntry) - 1;
        if(WorkIndex < WorkArray->TotalEntries)
        {
            //Extract work info into locals
            tile_work_entry* Work = WorkArray->Entries + WorkIndex;
            u32 CountX = Work->CountX;
            u32 CountY = Work->CountY;
            random_series Series = Work->RandomSeries;
            
            //Reset stats accumulators to 0
            Thread_TriangleTestsPassed = 0;
            Thread_TriangleTestsTotal = 0;
            
            //Execute work
            for(u32 y = Work->y; y < Work->y + CountY; y++)
            {
                f32 FilmY = (f32)y / OutputHeight * 2.0f - 1.0f;
                for(u32 x = Work->x; x < Work->x + CountX; x++)
                {
                    f32 FilmX = (f32)x / OutputWidth * 2.0f - 1.0f;
                    
                    f32 RayContrib = 1.0f / (f32)RaysPerPixel;
                    vec3 Color = vec3(0.0f);
                    For(Index, RaysPerPixel)
                    {
//                        f32 OffX = FilmX + RandNO(&Series) * HalfPixW;
//                        f32 OffY = FilmY + RandNO(&Series) * HalfPixH;
                        f32 OffX = FilmX + Samples[Index].x * HalfPixW;
                        f32 OffY = FilmY + Samples[Index].y * HalfPixH;
                        
                        vec3 RayOrigin = FilmCenter + OffX * HalfFilmW * CameraX + OffY * HalfFilmH * CameraY;
                        vec3 RayDirection = Normalize(CameraP - RayOrigin);
                        
                        //Raycast and accumulate color
                        Color = Color + RayCast(World, RayOrigin, RayDirection, Bounces, &Series) * RayContrib;
                        
                        InterlockedIncrement64(&Init->RaysCasted);
                    }
                    
                    //Output computed pixel color into SRGB texture
                    u32* OutputMemory = (u32*)OutputImage->Data;
                    u32* OutputPixel = OutputMemory + x + y * OutputWidth;
                    vec4 SRGBColor = ExactLinearToSRGB(vec4(Color, 1.0f));
                    *OutputPixel = ClampVec4ToRGBA(SRGBColor);
                }
                
                //Only the main thread prints stats
                if(Init->MainThreadId == ThreadId)
                {
                    Assert(Init->RaysCasted <= TotalRaysToCast);
                    f32 PercentageDone = (f32)Init->RaysCasted / TotalRaysToCast * 100.0f;
                    if((u32)PercentageDone > PercentageCounter)
                    {
                        PercentageCounter = (u32)PercentageDone;
                        printf("\rRay casting progress: %u%%", PercentageCounter);
                        fflush(stdout);
                    }
                }
            }
            
            //Update stats
            InterlockedAdd64((s64*)&Init->TriangleTestsPassed, Thread_TriangleTestsPassed);
            InterlockedAdd64((s64*)&Init->TriangleTestsTotal,  Thread_TriangleTestsTotal);
            
            //Increment work done counter
            InterlockedIncrement(&WorkArray->EntriesDone);
        }
        else
        {
            break;
        }
    }
    
    return 0;
}
