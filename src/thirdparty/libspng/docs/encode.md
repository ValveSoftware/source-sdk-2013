# Usage

The context must be created with the `SPNG_CTX_ENCODER` flag set.

Before any (implicit) write operation an output must be set with
[spng_set_png_stream()](context.md#spng_set_png_stream) or [spng_set_png_file()](context.md#spng_set_png_file).

Alternatively the `SPNG_ENCODE_TO_BUFFER` option can be enabled with [spng_set_option()](context.md#spng_set_option),
in this case the encoder will create and manage an internal output buffer,
it is freed by `spng_ctx_free()` unless it's retrieved with [`spng_get_png_buffer()`](#spng_get_png_buffer).

In all cases the PNG must be explicitly finalized, this can be done with the `SPNG_ENCODE_FINALIZE` flag or with a call to
[`spng_encode_chunks()`](#spng_encode_chunks) after the image has been encoded.

# Memory usage

On top of the overhead of the the context buffer the internal write buffer
may grow to the length of entire chunks or more than the length of
the PNG file when encoding to the internal buffer.

Encoding an image requires at least 2 rows to be kept in memory,
this may increase to 3 rows for future versions.

# Data types

## spng_encode_flags

```c
enum spng_encode_flags
{
    SPNG_ENCODE_PROGRESSIVE = 1, /* Initialize for progressive writes */
    SPNG_ENCODE_FINALIZE = 2, /* Finalize PNG after encoding image */
};
```

# API

# spng_encode_chunks()
```c
int spng_encode_chunks(spng_ctx *ctx)
```

Encode all stored chunks before or after the image data (IDAT) stream,
depending on the state of the encoder.

If the image is encoded this function will also finalize the PNG with the end-of-file (IEND) marker.

Calling this function before `spng_encode_image()` is optional.

# spng_encode_image()
```c
int spng_encode_image(spng_ctx *ctx, const void *img, size_t len, int fmt, int flags)
```

Encodes the image from `img` in the source format `fmt` to the specified PNG format set with [spng_set_ihdr()](chunk.md#spng_set_ihdr).

The width, height, color type, bit depth and interlace method must be set with [spng_set_ihdr()](chunk.md#spng_set_ihdr).

If the color type is set to `SPNG_COLOR_TYPE_INDEXED` a palette must be set with [spng_set_plte()](chunk.md#spng_set_plte).

`img` must point to a buffer of length `len`, `len` must be equal to the expected image
size for the given format `fmt`.

This function may call `spng_encode_chunks()`, writing any pending chunks before the image data.

If the `SPNG_ENCODE_FINALIZE` flag is set this function will call `spng_encode_chunks()` on the last
scanline/row, writing any pending chunks after the image data and finalize the PNG with the
end-of-file (IEND) marker before returning.
In most cases the only data after the image is the 12-byte IEND marker.

When the image is encoded to an internal buffer and the PNG is finalized
[spng_get_png_buffer()](#spng_get_png_buffer) will return the encoded buffer,
this must be freed by the user. If this function isn't called or an error is encountered
the internal buffer is freed by [spng_ctx_free()](context.md#spng_ctx_free).

## Supported format, flag combinations

| Input format   | PNG Format  | Flags | Notes                                  |
|----------------|-------------|-------|----------------------------------------|
| `SPNG_FMT_PNG` | Any format* | All   | Converted from host-endian if required |
| `SPNG_FMT_RAW` | Any format* | All   | No conversion (assumes big-endian)     |

\* Any combination of color type and bit depth defined in the [standard](https://www.w3.org/TR/2003/REC-PNG-20031110/#table111).

16-bit images are assumed to be host-endian except for `SPNG_FMT_RAW`.

The alpha channel is always [straight alpha](https://en.wikipedia.org/wiki/Alpha_compositing#Straight_versus_premultiplied),
premultiplied alpha is not supported.

Compression level and other options can be customized with [`spng_set_option()`](context.md#spng_set_option),
see [Encode options](encode.md#encode-options) for all options.
Note that encoder options are optimized based on PNG format and compression level,
overriding other options such as filtering may disable some of these optimizations.


## Progressive image encoding

If the `SPNG_ENCODE_PROGRESSIVE` flag is set the encoder will be initialized
with `fmt` and `flags` for progressive encoding, the values of `img`, `len` are ignored.

With the `SPNG_ENCODE_FINALIZE` flag set the PNG is finalized on the last scanline
or row.

Progressive encoding is straightforward when the image is not interlaced,
calling [spng_encode_row()](#spng_encode_row) for each row of the image will yield
the return value `SPNG_EOI` for the final row:

```c
int error;
size_t image_width = image_size / ihdr.height;

for(i = 0; i < ihdr.height; i++)
{
    void *row = image + image_width * i;

    error = spng_encode_row(ctx, row, image_width);

    if(error) break;
}

if(error == SPNG_EOI) /* success */
```

But for interlaced images (`spng_ihdr.interlaced_method` set to `1`)
rows are accessed multiple times and non-sequentially,
use [spng_get_row_info()](context.md#spng_get_row_info) to get the current row number:

```c
int error;
struct spng_row_info row_info;

do
{
    error = spng_get_row_info(ctx, &row_info);
    if(error) break;

    void *row = image + image_width * row_info.row_num;

    error = spng_encode_row(ctx, row, len);
}
while(!error)

if(error == SPNG_EOI) /* success */
```

# spng_encode_scanline()
```c
int spng_encode_scanline(spng_ctx *ctx, const void *scanline, size_t len)
```

Encodes `scanline`, this function is meant to be used for interlaced PNG's
with image data that is already split into multiple passes.

This function requires the encoder to be initialized by calling
`spng_encode_image()` with the `SPNG_ENCODE_PROGRESSIVE` flag set.

For the last scanline and subsequent calls the return value is `SPNG_EOI`.

# spng_encode_row()
```c
int spng_encode_row(spng_ctx *ctx, const void *row, size_t len)
```

Encodes `row`, interlacing it if necessary.

This function requires the encoder to be initialized by calling
`spng_encode_image()` with the `SPNG_ENCODE_PROGRESSIVE` flag set.

See also: [Progressive-image-encoding](#Progressive-image-encoding)

For the last row and subsequent calls the return value is `SPNG_EOI`.

If the image is not interlaced this function's behavior is identical to
`spng_encode_scanline()`.


# spng_get_png_buffer()
```c
void *spng_get_png_buffer(spng_ctx *ctx, size_t *len, int *error)
```

If `SPNG_ENCODE_TO_BUFFER` is enabled via [spng_set_option()](context.md#spng_set_option) the encoded buffer is returned,
it must be called after [spng_encode_image()](encode.md#spng_encode_image) and the PNG must be finalized.

On success the buffer must be freed by the user.

# Encode options

| Option                           | Default value             | Description                       |
|----------------------------------|---------------------------|-----------------------------------|
| `SPNG_IMG_COMPRESSION_LEVEL`     | `Z_DEFAULT_COMPRESSION`   | Set image compression level (0-9) |
| `SPNG_IMG_WINDOW_BITS`           | `15`*                     | Set image zlib window bits (9-15) |
| `SPNG_IMG_MEM_LEVEL`             | `8`                       | Set zlib `memLevel` for images    |
| `SPNG_IMG_COMPRESSION_STRATEGY`  | `Z_FILTERED`*             | Set image compression strategy    |
| `SPNG_TEXT_COMPRESSION_LEVEL`    | `Z_DEFAULT_COMPRESSION`   | Set text compression level (0-9)  |
| `SPNG_TEXT_WINDOW_BITS`          | `15`                      | Set text zlib window bits (9-15)  |
| `SPNG_TEXT_MEM_LEVEL`            | `8`                       | Set zlib `memLevel` for text      |
| `SPNG_TEXT_COMPRESSION_STRATEGY` | `Z_DEFAULT_STRATEGY`      | Set text compression strategy     |
| `SPNG_FILTER_CHOICE`             | `SPNG_FILTER_CHOICE_ALL`* | Configure or disable filtering    |
| `SPNG_ENCODE_TO_BUFFER`          | `0`                       | Encode to internal buffer         |

\* Option may be optimized if not set explicitly.

Options not listed here have no effect on encoders.
