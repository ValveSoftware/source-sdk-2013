# Data types

## spng_crc_action
```c
enum spng_crc_action
{
    /* Default for critical chunks */
    SPNG_CRC_ERROR = 0,

    /* Discard chunk, invalid for critical chunks,
       since v0.6.2: default for ancillary chunks */
    SPNG_CRC_DISCARD = 1,

    /* Ignore and don't calculate checksum */
    SPNG_CRC_USE = 2
};
```

## spng_decode_flags
```c
enum spng_decode_flags
{
    SPNG_DECODE_USE_TRNS = 1, /* Deprecated */
    SPNG_DECODE_USE_GAMA = 2, /* Deprecated */

    SPNG_DECODE_TRNS = 1, /* Apply transparency */
    SPNG_DECODE_GAMMA = 2, /* Apply gamma correction */
    SPNG_DECODE_PROGRESSIVE = 256 /* Initialize for progressive reads */
};
```

# Error handling

Decoding errors are divided into critical and non-critical errors.

See also: [General error handling](errors.md)

Critical errors are not recoverable, it should be assumed that decoding has
failed completely and any partial image output is invalid, although corrupted
PNG's may appear to decode to the same partial image every time this cannot be guaranteed.

A critical error will stop any further parsing, invalidate the context and return the
relevant error code, most functions check for a valid context state and return
`SPNG_EBADSTATE` for subsequent calls to prevent undefined behavior.
It is strongly recommended to check all return values.

Non-critical errors in a decoding context refers to file corruption issues
that can be handled in a deterministic manner either by ignoring checksums
or discarding invalid chunks.
The image is extracted consistently but may have lost color accuracy,
transparency, etc.

The default behavior is meant to emulate libpng for compatibility reasons
with existing images in the wild, most non-critical errors are ignored.
Configuring decoder strictness is currently limited to checksums.

* Invalid palette indices are handled as black, opaque pixels
* `tEXt`, `zTXt` chunks with non-Latin characters are considered valid
* Non-critical chunks are discarded if the:
    * Chunk CRC is invalid (`SPNG_CRC_DISCARD` is the default for ancillary chunks)
    * Chunk has an invalid DEFLATE stream, by default this includes Adler-32 checksum errors
    * Chunk has errors specific to the chunk type: unexpected length, out-of-range values, etc
* Critical chunks with either chunk CRC or Adler-32 errors will stop parsing (unless configured otherwise)
* Extra trailing image data is silently discarded
* No parsing or validation is done past the `IEND` end-of-file marker

Truncated PNG's and truncated image data is always handled as a critical error,
getting a partial image is possible with progressive decoding but is not
guaranteed to work in all cases. The decoder issues read callbacks that
can span multiple rows or even the whole image, partial reads are not processed.

Some limitations apply, spng will stop decoding if:

* An image row is larger than 4 GB
* Something causes arithmetic overflow (limited to extreme cases on 32-bit)

## Checksums

There are two types of checksums used in PNG's: 32-bit CRC's for chunk data and Adler-32
in DEFLATE streams.

Creating a context with the `SPNG_CTX_IGNORE_ADLER32` flag will cause Adler-32
checksums to be ignored by zlib, both in compressed metadata and image data.
Note this is only supported with zlib >= 1.2.11 and is not available when compiled against miniz.

Chunk CRC handling is configured with `spng_set_crc_action()`,
when `SPNG_CRC_USE` is used for either chunk types the Adler-32 checksums in DEFLATE streams
will be also ignored.
When set for both chunk types it has the same effect as `SPNG_CTX_IGNORE_ADLER32`,
this does not apply vice-versa.

Currently there are no distinctions made between Adler-32 checksum- and other errors
in DEFLATE streams, they're all mapped to the `SPNG_EZLIB` error code.

The libpng equivalent of `spng_set_crc_action()` is `png_set_crc_action()`,
it implements a subset of its features:

| libpng                 | spng               | Notes                   |
|------------------------|--------------------|-------------------------|
| `PNG_CRC_ERROR_QUIT`   | `SPNG_CRC_ERROR`   | Will not abort on error |
| `PNG_CRC_WARN_DISCARD` | `SPNG_CRC_DISCARD` | No warning system       |
| `PNG_CRC_QUIET_USE`    | `SPNG_CRC_USE`     |                         |


The `SPNG_CTX_IGNORE_ADLER32` context flag has the same effect as `PNG_IGNORE_ADLER32`
used with `png_set_option()`.

## Memory usage

The library will always allocate a context buffer,
if the input PNG is a stream or file it will also create a read buffer.

Decoding an image requires at least 2 rows to be kept in memory,
3 if the image is interlaced.

Gamma correction requires an additional 128KB for a lookup table if
the output format has 16 bits per channel (e.g. `SPNG_FMT_RGBA16`).

To limit memory usage for image decoding use `spng_set_image_limits()`
to set an image width/height limit.
This is the equivalent of `png_set_user_limits()`.

Moreover the size calculated by `spng_decoded_image_size()` can be checked
against a hard limit before allocating memory for the output image.

Chunks of arbitrary length (e.g. text, color profiles) take up additional memory,
`spng_set_chunk_limits()` is used to set hard limits on chunk length- and cache limits,
note that reaching either limit is handled as a fatal error.

Since v0.7.0 the `SPNG_CHUNK_COUNT_LIMIT` option controls how many chunks can be stored,
the default is `1000` and is configurable through [`spng_set_option()`](context.md#spng_set_option),
this limit is independent of the chunk cache limit.

# API

See also: [spng_set_png_stream()](context.md#spng_set_png_stream), [spng_set_png_file()](context.md#spng_set_png_file).

# spng_set_png_buffer()
```c
int spng_set_png_buffer(spng_ctx *ctx, void *buf, size_t size)
```

Set input PNG buffer, this can only be done once per context.

# spng_set_crc_action()
```c
int spng_set_crc_action(spng_ctx *ctx, int critical, int ancillary)
```

Set how chunk CRC errors should be handled for critical and ancillary chunks.

# spng_decoded_image_size()
```c
int spng_decoded_image_size(spng_ctx *ctx, int fmt, size_t *out)
```

Calculates decoded image buffer size for the given output format.

An input PNG must be set.

# spng_decode_chunks()
```c
int spng_decode_chunks(spng_ctx *ctx)
```

Decode all chunks before or after the image data (IDAT) stream,
depending on the state of the decoder.

If the image is decoded this function will read up to the end-of-file (IEND) marker.

Calling this function before `spng_decode_image()` is optional.

# spng_decode_image()
```c
int spng_decode_image(spng_ctx *ctx, void *out, size_t len, int fmt, int flags)
```

Decodes the PNG file and writes the image to `out`,
the image is converted from the PNG format to the destination format `fmt`.

Interlaced images are deinterlaced, 16-bit images are converted to host-endian.

`out` must point to a buffer of length `len`.

`len` must be equal to or greater than the number calculated with
`spng_decoded_image_size()` with the same output format.

If the `SPNG_DECODE_PROGRESSIVE` flag is set the decoder will be
initialized with `fmt` and `flags` for progressive decoding,
the values of `out`, `len` are ignored.

The `SPNG_DECODE_TNRS` flag is silently ignored if the PNG does not
contain a tRNS chunk or is not applicable for the color type.

This function can only be called once per context.

## Supported format, flag combinations

| PNG Format   | Output format     | Flags  | Notes                                         |
|--------------|-------------------|--------|-----------------------------------------------|
| Any format*  | `SPNG_FMT_RGBA8`  | All    | Convert from any PNG format and bit depth     |
| Any format   | `SPNG_FMT_RGBA16` | All    | Convert from any PNG format and bit depth     |
| Any format   | `SPNG_FMT_RGB8`   | All    | Convert from any PNG format and bit depth     |
| Gray <=8-bit | `SPNG_FMT_G8`     | None** | Only valid for 1, 2, 4, 8-bit grayscale PNG's |
| Gray 16-bit  | `SPNG_FMT_GA16`   | All**  | Only valid for 16-bit grayscale PNG's         |
| Gray <=8-bit | `SPNG_FMT_GA8`    | All**  | Only valid for 1, 2, 4, 8-bit grayscale PNG's |
| Any format   | `SPNG_FMT_PNG`    | None** | The PNG's format in host-endian               |
| Any format   | `SPNG_FMT_RAW`    | None   | The PNG's format in big-endian                |


\* Any combination of color type and bit depth defined in the [standard](https://www.w3.org/TR/2003/REC-PNG-20031110/#table111).

\*\* Gamma correction is not implemented

The `SPNG_DECODE_PROGRESSIVE` flag is supported in all cases.

The alpha channel is always [straight alpha](https://en.wikipedia.org/wiki/Alpha_compositing#Straight_versus_premultiplied),
premultiplied alpha is not supported.

## Progressive image decoding

If the `SPNG_DECODE_PROGRESSIVE` flag is set the decoder will be initialized
with `fmt` and `flags` for progressive decoding, the values of `img`, `len` are ignored.

Progressive decoding is straightforward when the image is not interlaced,
calling [spng_decode_row()](#spng_decode_row) for each row of the image will yield
the return value `SPNG_EOI` for the final row:

```c
int error;
size_t image_width = image_size / ihdr.height;

for(i = 0; i < ihdr.height; i++)
{
    void *row = image + image_width * i;

    error = spng_decode_row(ctx, row, image_width);

    if(error) break;
}

if(error == SPNG_EOI) /* success */
```

But for interlaced images rows are accessed multiple times and non-sequentially,
use [spng_get_row_info()](context.md#spng_get_row_info) to get the current row number:

```c
int error;
struct spng_row_info row_info;

do
{
    error = spng_get_row_info(ctx, &row_info);
    if(error) break;

    void *row = image + image_width * row_info.row_num;

    error = spng_decode_row(ctx, row, len);
}
while(!error)

if(error == SPNG_EOI) /* success */
```

This is the recommended solution in all cases, for non-interlaced images `row_num` will increase
linearly.

# spng_decode_scanline()
```c
int spng_decode_scanline(spng_ctx *ctx, void *out, size_t len)
```

Decodes a scanline to `out`.

This function requires the decoder to be initialized by calling
`spng_decode_image()` with the `SPNG_DECODE_PROGRESSIVE` flag set.

The widest scanline is the decoded image size divided by `ihdr.height`.

For the last scanline and subsequent calls the return value is `SPNG_EOI`.

# spng_decode_row()
```c
int spng_decode_row(spng_ctx *ctx, void *out, size_t len)
```

Decodes and deinterlaces a scanline to `out`.

This function requires the decoder to be initialized by calling
`spng_decode_image()` with the `SPNG_DECODE_PROGRESSIVE` flag set.

The width of the row is the decoded image size divided by `ihdr.height`.

For interlaced images rows are accessed multiple times and non-sequentially,
use [spng_get_row_info()](context.md#spng_get_row_info) to get the current row number.

For the last row and subsequent calls the return value is `SPNG_EOI`.

If the image is not interlaced this function's behavior is identical to
`spng_decode_scanline()`.

# Decode options

| Option                       | Default value | Description                                              |
|------------------------------|---------------|----------------------------------------------------------|
| `SPNG_KEEP_UNKNOWN_CHUNKS`   | `0`           | Set to keep or discard unknown chunks                    |
| `SPNG_IMG_COMPRESSION_LEVEL` | `-1`          | May expose an estimate (0-9) after `spng_decode_image()` |
| `SPNG_IMG_WINDOW_BITS`       | `15`*         | Set zlib window bits used for image decompression        |
| `SPNG_CHUNK_COUNT_LIMIT`     | `1000`        | Limit shared by both known and unknown chunks            |

\* Option may be optimized if not set explicitly.

Options not listed here have no effect on decoders.
