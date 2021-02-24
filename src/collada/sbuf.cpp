typedef struct SbufHdr
{
    size_t length;
    size_t capacity;
    char buffer[0];
} SbufHdr;

//Private API
#define sbuf__hdr(b) ((SbufHdr*)(((char*)(b)) - sizeof(SbufHdr)))
#define sbuf__fits1(b) ((b) && (SbufCap(b) > SbufLen(b)))
#define sbuf__push(b, x) ((b)[sbuf__hdr(b)->length++] = (x))

//Public API
#define _sbuf_
#define SbufLen(b) ((b) ? sbuf__hdr(b)->length : 0)
#define SbufCap(b) ((b) ? sbuf__hdr(b)->capacity : 0)
#define SbufEnd(b) ((b) ? b + SbufLen(b) : 0)
#define SbufPop(b) ((b) ? sbuf__hdr(b)->length--, (b) + SbufLen(b) : 0)
#define SbufPopN(b, n) ((b) ? sbuf__hdr(b)->length -= (n), (b) + SbufLen(b) : 0)
#define SbufFree(b) (b) ? Free(sbuf__hdr(b)), b = 0 : 0



internal void
sbuf__grow(void** b, u32 size, u32 count) //count is added to current cap
{
    size_t target_capacity = SbufCap(*b) + count;
    size_t new_capacity = MAX(((SbufCap(*b) * 2)), 1);
    while(new_capacity < target_capacity)
    {
        new_capacity = new_capacity * 2;
    }
    
    size_t new_size = size * new_capacity + sizeof(SbufHdr);
    SbufHdr* new_buffer = (SbufHdr*)ZeroAlloc(new_size);
    
    void* header = sbuf__hdr(*b);
    if(*b)
    {
        SbufHdr* old_buffer = sbuf__hdr(*b);
        memcpy(new_buffer, sbuf__hdr(*b), SbufLen(*b) * size + sizeof(SbufHdr));
        Free(old_buffer);
    }
    else
    {
        new_buffer->length = 0;
    }
    
    new_buffer->capacity = new_capacity;
    *b = new_buffer->buffer;
}

#ifndef __cplusplus

#define SbufPush(b, ...) (!sbuf__fits1(b) ? sbuf__grow(&(b), sizeof(__VA_ARGS__), 1), sbuf__push(b , __VA_ARGS__) : sbuf__push(b, __VA_ARGS__))
//#define SbufPushN(b, count) (count != 0 ? (sbuf__grow(&(b), sizeof(*(b)), count), sbuf__hdr(b)->length += count: 0)
#define SbufReserve(b, count) (count != 0 ? (sbuf__grow(&(b), sizeof(*(b)), count) : 0)

#else

template<class T> void
SbufReserve(T*& b, u32 count)
{
    sbuf__grow((void**)&b, sizeof(T), count);
}

template<class T> void
SbufPushN(T*& b, u32 count)
{
    if(SbufCap(b) - SbufLen(b) < count)
    {
        sbuf__grow((void**)&b, sizeof(T), count);
    }
    sbuf__hdr(b)->length += count;
}

template<class T> void
SbufPush(T*& b, T x)
{
    if(!sbuf__fits1(b))
    {
        sbuf__grow((void**)&b, sizeof(T), 1);
        sbuf__push(b, x);
    }
    else
    {
        sbuf__push(b,x);
    }
}

template<class T> void
SbufPushFront(T*& b, T x)
{
    if(!sbuf__fits1(b))
    {
        sbuf__grow((void**)&b, sizeof(T), 1);
    }
    
    for(u32 Index = SbufLen(b); Index > 0; Index--)
    {
        b[Index] = b[Index - 1];
    }
    
    sbuf__hdr(b)->length++;
    b[0] = x;
}

#endif
