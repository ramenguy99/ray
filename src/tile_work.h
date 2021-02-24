struct tile_work_entry
{
    u32 x;
    u32 y;
    u32 CountX;
    u32 CountY;
    random_series RandomSeries;
};

struct tile_work_array
{
    tile_work_entry* Entries;
    volatile u32 CurrentEntry;
    volatile u32 EntriesDone;
    u32 TotalEntries;
};

struct tile_worker_thread_init
{
    tile_work_array* WorkArray;
    
    //Output settings (read only)
    u32 OutputWidth;
    u32 OutputHeight;
    u32 RaysPerPixel;
    u32 RayBounces;
    vec2 Samples[MAX_RAYS_PER_PIXEL];
    
    //Scene info (read only)
    world* World;
    vec3 CameraP;
    vec3 CameraX;
    vec3 CameraY;
    vec3 CameraZ;
    
    //Output data (shared but written without overlap)
    image_data* OutputImage;
    
    //Stats
    volatile s64 RaysCasted;
    volatile s64 TriangleTestsPassed;
    volatile s64 TriangleTestsTotal;
    
    //Used to identify the printer thread
    thread_id MainThreadId;
};
