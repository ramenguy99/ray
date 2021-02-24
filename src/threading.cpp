
#ifdef _WIN32
#include "windows.h"
typedef DWORD thread_id;

#define THREAD_PROC(name) DWORD WINAPI name(void* Data)

#else

#include <pthread.h>
typedef pthread_t thread_id;
#define THREAD_PROC(name) void* name(void* Data)
#define GetCurrentThreadId pthread_self

u32 InterlockedIncrement(volatile u32* ptr)
{
    return __sync_add_and_fetch(ptr, 1);
}

s64 InterlockedIncrement64(volatile s64* ptr)
{
    return __sync_add_and_fetch(ptr, 1);
}

s64 InterlockedAdd64(volatile s64* ptr, s64 value)
{
    return __sync_add_and_fetch(ptr, value);
}

#endif



typedef THREAD_PROC(thread_proc);

internal void
CreateWorkerThread(thread_proc Proc, void* Data)
{
#ifdef _WIN32
    CreateThread(0, 0, Proc, Data, 0, 0);
#else
    pthread_t Thread;
    pthread_create(&Thread, 0, Proc, Data);
#endif
}