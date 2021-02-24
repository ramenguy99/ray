
// WINDOWS
#ifdef _WIN32

#include "windows.h"

typedef s64 timestamp;

inline s64
GetCurrentCounter()
{
    LARGE_INTEGER CurrentCounter;
    QueryPerformanceCounter(&CurrentCounter);
    return CurrentCounter.QuadPart;
}

inline float32
GetSecondsElapsed(s64 CounterBegin, s64 CounterEnd)
{
    static s64 PerformanceFrequency;
    if(!PerformanceFrequency)
    {
        LARGE_INTEGER Frequency;
        QueryPerformanceFrequency(&Frequency);
        PerformanceFrequency = Frequency.QuadPart;
    }
    
    s64 CounterElapsed = CounterEnd - CounterBegin;
    f32 SecondsElapsed = (f32)CounterElapsed / PerformanceFrequency;
    
    return SecondsElapsed;
}


internal void
FreeFileMemory(void* Memory)
{
    VirtualFree(Memory, 0, MEM_RELEASE);
}


//Appends a 0 to the returned buffer for c string compatibility
internal void* 
ReadFileAsString(char* FileName, u32* OutBytesRead = 0)
{
    void* Result = 0;
    HANDLE FileHandle;
    FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0,0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if(GetFileSizeEx(FileHandle, &FileSize))
        {
            Assert(FileSize.QuadPart <= 0xFFFFFFFF);
            uint32 FileSize32 = (uint32)FileSize.QuadPart;
            Result = VirtualAlloc(0, FileSize32 + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if(Result)
            {
                DWORD BytesRead = 0;
                if(ReadFile(FileHandle, Result, FileSize32, &BytesRead,0)
                   && FileSize32 == BytesRead)
                {
                    //SUCCESS
                    if(OutBytesRead) 
                    {
                        *OutBytesRead = BytesRead;
                    }
                }
                else
                {
                    //ReadFile failed
                    FreeFileMemory(Result);
                    Result = 0;
                }
            }
            else
            {
                //VirtualAlloc failed
            }
        }
        else
        {
            //GetFileSize failed
        }
        CloseHandle(FileHandle);
    }
    else
    {
        //CreateFile failed (probably file doesn't exist);
    }
    
    return Result;
}

#else


//LINUX

typedef timespec timestamp;
#include <unistd.h>
#include <time.h>

inline timespec
GetCurrentCounter()
{
    timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    
    return time;
}

inline f32
GetSecondsElapsed(timespec Begin, timespec End)
{
    f32 SecondsElapsed = End.tv_sec - Begin.tv_sec;
    SecondsElapsed += (End.tv_nsec - Begin.tv_nsec) * (1.0 / 1e9);
    
    return SecondsElapsed;
}

internal void* 
ReadFileAsString(char* FileName, u32* OutBytesRead = 0)
{
    FILE *f = fopen(FileName, "rb");
    if(!f) return 0;
    
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  /* same as rewind(f); */
    
    char *string = (char*)malloc(fsize + 1);
    if(!string) return 0;
    
    size_t bytes_read = fread(string, 1, fsize, f);
    fclose(f);
    
    if(bytes_read != fsize) {
        free(string);
        return 0;
    }
    
    string[fsize] = 0;
    
    return string;
}

internal void
FreeFileMemory(void* Memory)
{
    free(Memory);
}

#define Sleep(i) usleep(i * 1000)

#endif