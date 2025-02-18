#ifndef SPNG_FRAMAC_STUBS_H
#define SPNG_FRAMAC_STUBS_H

#include <stddef.h>

#define ZLIB_VERNUM 0x1290

#define Z_NO_FLUSH      0
#define Z_PARTIAL_FLUSH 1
#define Z_SYNC_FLUSH    2
#define Z_FULL_FLUSH    3
#define Z_FINISH        4
#define Z_BLOCK         5
#define Z_TREES         6

#define Z_OK            0
#define Z_STREAM_END    1
#define Z_NEED_DICT     2
#define Z_ERRNO        (-1)
#define Z_STREAM_ERROR (-2)
#define Z_DATA_ERROR   (-3)
#define Z_MEM_ERROR    (-4)
#define Z_BUF_ERROR    (-5)
#define Z_VERSION_ERROR (-6)

#define Z_NULL 0

typedef void* (*alloc_func)(void *opaque, unsigned int items, unsigned int size);
typedef void (*free_func)(void *opaque, void *address);

typedef struct z_stream_s
{
    const unsigned char *next_in;
    unsigned int avail_in;
    unsigned long total_in;

    unsigned char *next_out;
    unsigned int avail_out;
    unsigned long total_out;

    alloc_func zalloc;
    free_func  zfree;
    void *opaque;

    int     data_type;
    unsigned long adler;
}z_stream;

typedef z_stream *z_streamp;

int inflateInit(z_streamp strm)
{
    if(strm == NULL) return 1;

    return 0;
}

int inflate(z_streamp stream, int flush)
{
    if(stream==NULL) return Z_STREAM_ERROR;

    if(stream->avail_in == 0 || stream->avail_out == 0) return Z_BUF_ERROR;

    stream->next_in += stream->avail_in;

    stream->next_out += stream->avail_out;

    return 0;
}

int inflateEnd(z_streamp stream)
{
    if(stream==NULL) return 1;

    return 0;
}

int inflateValidate(z_streamp stream, int b)
{
    return 0;
}

unsigned long crc32(unsigned long crc, const unsigned char *buf, unsigned int len)
{
    if(buf==NULL) return 0;
    return crc+len;
}

#endif /* SPNG_FRAMAC_STUBS_H */
