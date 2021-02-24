typedef struct intern
{
    size_t Length;
    struct intern* Next;
    char String[];
}intern;

typedef struct intern_map
{
    arena Arena;
    map Map;
} intern_map;

internal void
FreeInternMap(intern_map* InternMap)
{
    FreeArena(&InternMap->Arena);
    FreeMap(&InternMap->Map);
}

internal char* 
InternStringRange(intern_map* InternMap, char* String, char* End)
{
    u32 Length = (u32)(End - String);
    u64 Hash = HashBytes(String, Length);
    void* Key = Hash ? (void*)Hash : (void*)1;
    intern* Intern = (intern*)MapGet(&InternMap->Map, Key);
    
    for(intern* It = Intern; It != 0; It = It->Next)
    {
        if(It->Length == Length && strncmp(It->String, String, Length) == 0)
        {
            return It->String;
        }
    }
    
    intern* NewIntern = (intern*)ArenaAlloc(&InternMap->Arena, offsetof(intern, String) + Length + 1); //+1 for terminating 0
    NewIntern->Length = Length;
    NewIntern->Next = Intern;
    memcpy(NewIntern->String, String, Length);
    NewIntern->String[Length] = 0;
    
    MapPut(&InternMap->Map, Key, NewIntern);
    return NewIntern->String;    
}

internal char* 
InternString(intern_map* InternMap, char* String)
{
    return InternStringRange(InternMap, String, String + strlen(String));    
}

internal void 
TestStringIntern()
{
    intern_map InternMap = {0};
    
    char* Test = InternString(&InternMap, "Hello there");
    char* Test2 = InternString(&InternMap, "Hello there");
    Assert(Test == Test2);
    char* Test3 = InternString(&InternMap, "Hello!");
    char* Test4 = InternString(&InternMap, "Hello");
    Assert(Test != Test3 && Test3 != Test4);
}
