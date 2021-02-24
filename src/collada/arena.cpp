#define ARENA_BLOCK_SIZE 1024

typedef struct arena
{
    u8* Top; //Top of current block
    u8* End;   //End of current block
    _sbuf_ u8** Blocks; //Keep track of allocated blocks to allow freeing.
} arena;

//return the size of the current arena block
#define ArenaSize(A) (size_t)(A->End - A->Top)

internal void 
ArenaGrow(arena* Arena, size_t MinimumSize)
{
    size_t AllocSize = MAX(ARENA_BLOCK_SIZE, MinimumSize);
    Arena->Top = (u8*)ZeroAlloc(AllocSize);
    Arena->End = Arena->Top + AllocSize;
    
    SbufPush(Arena->Blocks, Arena->Top);
}

internal void* 
ArenaAlloc(arena* Arena, size_t Size)
{
    size_t RemainingSize = ArenaSize(Arena);
    if(RemainingSize < Size)
    {
        ArenaGrow(Arena, Size);
        Assert(ArenaSize(Arena) >= Size);
    }
    void* Result = Arena->Top;
    Arena->Top = Arena->Top + Size;
    return Result;
}

internal void 
FreeArena(arena* Arena)
{
    for(u8** It = Arena->Blocks; It != SbufEnd(Arena->Blocks); It++)
    {
        Free(*It);
    }
    SbufFree(Arena->Blocks);
}


internal void 
TestArena()
{
    arena Arena = {0};
    int* Test = (int*)ArenaAlloc(&Arena, sizeof(int));
    *Test = 5;
    int* Test2 = (int*)ArenaAlloc(&Arena, sizeof(int) * 1024);
    Assert(*Test == 5);
    
    FreeArena(&Arena);
}

#undef ArenaSize
