# Data types

# spng_ctx
```c
typedef struct spng_ctx spng_ctx;
```

   Context handle.

!!!note
    The context handle has no public members.


# spng_ctx_flags

```c
enum spng_ctx_flags
{
    SPNG_CTX_IGNORE_ADLER32 = 1, /* Ignore checksum in DEFLATE streams */
    SPNG_CTX_ENCODER = 2 /* Create an encoder context */
};
```

# spng_read_fn
```c
typedef int spng_read_fn(spng_ctx *ctx, void *user, void *dest, size_t length)
```

Type definition for callback passed to `spng_set_png_stream()` for decoders.

A read callback function should copy `length` bytes to `dest` and return 0 or
`SPNG_IO_EOF`/`SPNG_IO_ERROR` on error.


# spng_write_fn
```c
typedef int spng_write_fn(spng_ctx *ctx, void *user, void *src, size_t length)
```

Type definition for callback passed to `spng_set_png_stream()` for encoders.

The write callback should process `length` bytes and return 0 or `SPNG_IO_ERROR` on error.


# spng_format
```c
enum spng_format
{
    SPNG_FMT_RGBA8 = 1,
    SPNG_FMT_RGBA16 = 2,
    SPNG_FMT_RGB8 = 4,

    SPNG_FMT_GA8 = 16,
    SPNG_FMT_GA16 = 32,
    SPNG_FMT_G8 = 64,

    /* No conversion or scaling */
    SPNG_FMT_PNG = 256, /* host-endian */
    SPNG_FMT_RAW = 512  /* big-endian */
};
```
!!! note
    The channels are always in [byte-order](https://en.wikipedia.org/wiki/RGBA_color_model#RGBA8888) representation.

    The alpha channel is always [straight alpha](https://en.wikipedia.org/wiki/Alpha_compositing#Straight_versus_premultiplied),
    premultiplied alpha is not supported.

# spng_filter
```c
enum spng_filter
{
    SPNG_FILTER_NONE = 0,
    SPNG_FILTER_SUB = 1,
    SPNG_FILTER_UP = 2,
    SPNG_FILTER_AVERAGE = 3,
    SPNG_FILTER_PAETH = 4
};
```

# spng_row_info
```c
struct spng_row_info
{
    uint32_t scanline_idx;
    uint32_t row_num;
    int pass;
    uint8_t filter;
};
```

Contains row and scanline information, used for progressive decoding and encoding.

# spng_option
```c
enum spng_option
{
    SPNG_KEEP_UNKNOWN_CHUNKS = 1,

    SPNG_IMG_COMPRESSION_LEVEL,
    SPNG_IMG_WINDOW_BITS,
    SPNG_IMG_MEM_LEVEL,
    SPNG_IMG_COMPRESSION_STRATEGY,

    SPNG_TEXT_COMPRESSION_LEVEL,
    SPNG_TEXT_WINDOW_BITS,
    SPNG_TEXT_MEM_LEVEL,
    SPNG_TEXT_COMPRESSION_STRATEGY,

    SPNG_FILTER_CHOICE,
    SPNG_CHUNK_COUNT_LIMIT,
    SPNG_ENCODE_TO_BUFFER,
};
```

# spng_filter_choice
```c
enum spng_filter_choice
{
    SPNG_DISABLE_FILTERING = 0,
    SPNG_FILTER_CHOICE_NONE = 8,
    SPNG_FILTER_CHOICE_SUB = 16,
    SPNG_FILTER_CHOICE_UP = 32,
    SPNG_FILTER_CHOICE_AVG = 64,
    SPNG_FILTER_CHOICE_PAETH = 128,
    SPNG_FILTER_CHOICE_ALL = (8|16|32|64|128)
};
```

# API

# spng_ctx_new()
```c
spng_ctx *spng_ctx_new(int flags)
```

Creates a new context.

# spng_ctx_new2()
```c
spng_ctx *spng_ctx_new2(struct spng_alloc *alloc, int flags)
```

Creates a new context with a custom memory allocator, it is passed to zlib.

`alloc` and its members must be non-NULL.

# spng_ctx_free()
```c
void spng_ctx_free(spng_ctx *ctx)
```

Releases context resources.

# spng_set_png_stream()
```c
int spng_set_png_stream(spng_ctx *ctx, spng_rw_fn *rw_func, void *user)
```

Set input PNG stream or output PNG stream, depending on context type.

This can only be done once per context.

!!! info
    PNG's are read up to the file end marker, this is identical behavior to libpng.

# spng_set_png_file()
```c
int spng_set_png_file(spng_ctx *ctx, FILE *file)
```

Set input PNG file or output PNG file, depending on context type.

This can only be done once per context.

# spng_set_image_limits()
```c
int spng_set_image_limits(spng_ctx *ctx, uint32_t width, uint32_t height)
```

Set image width and height limits, these may not be larger than 2<sup>31</sup>-1.

# spng_get_image_limits()
```c
int spng_get_image_limits(spng_ctx *ctx, uint32_t *width, uint32_t *height)
```

Get image width and height limits.

`width` and `height` must be non-NULL.

# spng_set_chunk_limits()
```c
int spng_set_chunk_limits(spng_ctx *ctx, size_t chunk_size, size_t cache_limit)
```

Set chunk size and chunk cache limits, the default chunk size limit is 2<sup>31</sup>-1,
the default chunk cache limit is `SIZE_MAX`.

Reaching either limit while decoding is handled as an out-of-memory error.

!!!note
    This can only be used for limiting memory usage, most standard chunks
    do not require additional memory and are stored regardless of these limits.

# spng_get_chunk_limits()
```c
int spng_get_chunk_limits(spng_ctx *ctx, size_t *chunk_size, size_t *cache_limit)
```

Get chunk size and chunk cache limits.

# spng_get_row_info()
```c
int spng_get_row_info(spng_ctx *ctx, struct spng_row_info *row_info)
```

Copies the current, to-be-decoded (or to-be-encoded) row's information to `row_info`.

# spng_set_option()
```c
int spng_set_option(spng_ctx *ctx, enum spng_option option, int value)
```

Set `option` to the specified `value`.

For details see [Decode options](decode.md#decode-options) and [Encode options](encode.md#encode-options).

# spng_get_option()
```c
int spng_get_option(spng_ctx *ctx, enum spng_option option, int *value)
```

Get the value for the specified `option`.

For details see [Decode options](decode.md#decode-options) and [Encode options](encode.md#encode-options).
