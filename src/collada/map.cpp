typedef struct map
{
    void** Keys;
    void** Values;
    
    size_t Length;
    size_t Capacity;
}map;

void MapGrow(map* Map, size_t NewCapacity);

u64 Hashu64(u64 x) 
{
    x *= 0xff51afd7ed558ccd;
    x ^= x >> 32;
    return x;
}

u64 HashPointer(void* Key)
{
    return Hashu64((u64)Key);
}

u64 HashMix(u64 x, u64 y) 
{
    x ^= y;
    x *= 0xff51afd7ed558ccd;
    x ^= x >> 32;
    return x;
}

u64 HashBytes(void *Bytes, size_t Length) {
    u64 x = 0xcbf29ce484222325;
    char *Buffer = (char *)Bytes;
    for (size_t Index = 0; Index < Length; Index++) {
        x ^= Buffer[Index];
        x *= 0x100000001b3;
        x ^= x >> 32;
    }
    return x;
}

void* MapGet(map* Map, void* Key)
{
    if(Map->Length == 0)
    {
        return 0;
    }
    
    size_t Index = (size_t)HashPointer(Key);
    Assert(Map->Length < Map->Capacity);
    
    while(1)
    {
        Index &= Map->Capacity -1;
        if(Map->Keys[Index] == Key)
        {
            return Map->Values[Index];
        }
        else if(Map->Keys[Index] == 0)
        {
            return 0;
        }
        Index++;
    }
    
    return 0;
}

void MapPut(map* Map, void* Key, void* Value)
{
    Assert(Key);
    if(Value == 0)
    {
        return;
    }
    
    if(2 * Map->Length >= Map->Capacity)
    {
        MapGrow(Map, 2 * Map->Capacity);
    }
    Assert(Map->Length * 2 < Map->Capacity);
    Assert(IS_POW2(Map->Capacity));
    
    size_t Index = HashPointer(Key);
    
    // NOTE(Flame): Keep going until we find an empty spot or the same key.
    while(1)
    {
        Index &= Map->Capacity - 1;
        if(Map->Keys[Index] == 0)
        {
            Map->Length++;
            Map->Keys[Index] = Key;
            Map->Values[Index] = Value;
            
            return;
        }
        else if(Map->Keys[Index] == Key)
        {
            Map->Values[Index] = Value;
            
            return;
        }
        
        Index++;
    }
}

void MapGrow(map* Map, size_t NewCapacity)
{
    NewCapacity = MAX(16, NewCapacity);
    map NewMap;
    NewMap.Keys = (void**)ZeroAlloc(NewCapacity * sizeof(void*)); //Needs to be zeros
    NewMap.Values = (void**)ZeroAlloc(NewCapacity * sizeof(void*)); //Doesn't need zeros
    NewMap.Length = 0;
    NewMap.Capacity = NewCapacity;
    
    for(size_t Index = 0; Index < Map->Capacity; Index++)
    {
        if(Map->Keys[Index] != 0)
        {
            MapPut(&NewMap, Map->Keys[Index], Map->Values[Index]);
        }
    }
    
    Free(Map->Keys);
    Free(Map->Values);
    *Map = NewMap;
}

void FreeMap(map* Map)
{
    if(Map->Keys) {
        Free(Map->Keys);
    }
    if(Map->Values) {
        Free(Map->Values);
    }
}

void TestMap()
{
    
    map Map = {0};
    enum { N = 1024 };
    for (size_t i = 1; i < N; i++) {
        MapPut(&Map, (void *)i, (void *)(i+1));
    }
    for (size_t i = 1; i < N; i++) {
        void *val = MapGet(&Map, (void *)i);
        Assert(val == (void *)(i+1));
    }
    
    FreeMap(&Map);
}
