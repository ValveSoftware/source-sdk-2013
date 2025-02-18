# Contexts

The context handle `spng_ctx` is an opaque datatype that holds all information,
there is no separate info struct.

Create a new context with [`spng_ctx_new()`](context.md#spng_ctx_new) or [`spng_ctx_new2()`](context.md#spng_ctx_new2) to set a custom memory allocator.

Contexts are decoders by default, to create an encoder use `SPNG_CTX_ENCODER` as context flag.

`spng_ctx_free()` frees all context data.


# Error handling

All functions return zero on success and non-zero on error,
decoding or encoding errors will invalidate the context and most subequent function calls will return
`SPNG_EBADSTATE` to signal this.

See also: [Decoder error handling](decode.md#error-handling)

# Limits

!!! note
    `cache_max` refers to an overall memory usage limit in spng.

| libpng                       | spng                                                | Notes                    |
|------------------------------|-----------------------------------------------------|--------------------------|
| `png_set_user_limits()`      | [`spng_set_image_limits()`](#spng_set_image_limits) |                          |
| `png_get_user_width_max()`   | [`spng_get_image_limits()`](#spng_get_image_limits) |                          |
| `png_get_user_height_max()`  | [`spng_get_image_limits()`](#spng_get_image_limits) |                          |
| `png_set_chunk_malloc_max()` | [`spng_set_chunk_limits()`](#spng_set_chunk_limits) | `chunk_size` argument    |
| `png_get_chunk_malloc_max()` | [`spng_get_chunk_limits()`](#spng_get_chunk_limits) | `chunk_size` argument    |
| `png_set_chunk_cache_max()`  | [`spng_set_option()`](context.md#spng_set_option)   | `SPNG_CHUNK_COUNT_LIMIT` |

# Image information and chunk data

!!! info
    Image dimensions and other basic properties can be retrieved with [`spng_get_ihdr()`](chunk.md#spng_get_ihdr).

!!! note
    Most chunk data is copied except for arbitrary length chunks (eXIf, text, sPLT, unknown chunks).

    Chunk lists are not copied, the reference must stay valid for the lifetime of the context.

    To retrieve chunk lists for text, sPLT and unknown chunks the user must allocate buffers for them after querying the list size,
    unlike with libpng the internal list pointers are not accessible.

| libpng                       | spng                                                            | Notes                 |
|------------------------------|-----------------------------------------------------------------|-----------------------|
| `png_get_IHDR()`             | [`spng_get_ihdr()`](chunk.md#spng_get_ihdr)                     | Get image information |
| `png_get_PLTE()`             | [`spng_get_plte()`](chunk.md#spng_get_plte)                     |                       |
| `png_get_tRNS()`             | [`spng_get_trns()`](chunk.md#spng_get_trns)                     |                       |
| `png_get_cHRM()`             | [`spng_get_chrm()`](chunk.md#spng_get_chrm)                     |                       |
| `png_get_cHRM_fixed()`       | [`spng_get_chrm_int()`](chunk.md#spng_get_chrm_int)             |                       |
| `png_get_gAMA()`             | [`spng_get_gama()`](chunk.md#spng_get_gama)                     |                       |
| `png_get_gAMA_fixed()`       | [`spng_get_gama_int()`](chunk.md#spng_get_gama_int)             |                       |
| `png_get_iCCP()`             | [`spng_get_iccp()`](chunk.md#spng_get_iccp)                     |                       |
| `png_get_sBIT()`             | [`spng_get_sbit()`](chunk.md#spng_get_sbit)                     |                       |
| `png_get_sRGB()`             | [`spng_get_srgb()`](chunk.md#spng_get_srgb)                     |                       |
| `png_get_text()`             | [`spng_get_text()`](chunk.md#spng_get_text)                     |                       |
| `png_get_bKGD()`             | [`spng_get_bkgd()`](chunk.md#spng_get_bkgd)                     |                       |
| `png_get_hIST()`             | [`spng_get_hist()`](chunk.md#spng_get_hist)                     |                       |
| `png_get_pHYs()`             | [`spng_get_phys()`](chunk.md#spng_get_phys)                     |                       |
| `png_get_sPLT()`             | [`spng_get_splt()`](chunk.md#spng_get_splt)                     |                       |
| `png_get_tIME()`             | [`spng_get_time()`](chunk.md#spng_get_time)                     |                       |
| `png_get_oFFs()`             | [`spng_get_offs()`](chunk.md#spng_get_offs)                     |                       |
| `png_get_eXIf_1()`           | [`spng_get_exif()`](chunk.md#spng_get_exif)                     |                       |
| `png_get_sCAL())`            | N/A                                                             | Not supported         |
| `png_set_IHDR()`             | [`spng_set_ihdr()`](chunk.md#spng_set_ihdr)                     |                       |
| `png_set_PLTE()`             | [`spng_set_plte()`](chunk.md#spng_set_plte)                     |                       |
| `png_set_tRNS()`             | [`spng_set_trns()`](chunk.md#spng_set_trns)                     |                       |
| `png_set_cHRM()`             | [`spng_set_chrm()`](chunk.md#spng_set_chrm)                     |                       |
| `png_set_cHRM_fixed()`       | [`spng_set_chrm_int()`](chunk.md#spng_set_chrm_int)             |                       |
| `png_set_gAMA()`             | [`spng_set_gama()`](chunk.md#spng_set_gama)                     |                       |
| `png_set_gAMA_fixed()`       | [`spng_set_gama_int()`](chunk.md#spng_set_gama_int)             |                       |
| `png_set_iCCP()`             | [`spng_set_iccp()`](chunk.md#spng_set_iccp)                     |                       |
| `png_set_sBIT()`             | [`spng_set_sbit()`](chunk.md#spng_set_sbit)                     |                       |
| `png_set_sRGB()`             | [`spng_set_srgb()`](chunk.md#spng_set_srgb)                     |                       |
| `png_set_text()`             | [`spng_set_text()`](chunk.md#spng_set_text)                     |                       |
| `png_set_bKGD()`             | [`spng_set_bkgd()`](chunk.md#spng_set_bkgd)                     |                       |
| `png_set_hIST()`             | [`spng_set_hist()`](chunk.md#spng_set_hist)                     |                       |
| `png_set_pHYs()`             | [`spng_set_phys()`](chunk.md#spng_set_phys)                     |                       |
| `png_set_sPLT()`             | [`spng_set_splt()`](chunk.md#spng_set_splt)                     |                       |
| `png_set_tIME()`             | [`spng_set_time()`](chunk.md#spng_set_time)                     |                       |
| `png_set_oFFs()`             | [`spng_set_offs()`](chunk.md#spng_set_offs)                     |                       |
| `png_set_eXIf_1()`           | [`spng_set_exif()`](chunk.md#spng_set_exif)                     |                       |
| `png_set_sCAL())`            | N/A                                                             | Not supported         |
| `png_get_unknown_chunks()`   | [`spng_get_unknown_chunks()`](chunk.md#spng_get_unknown_chunks) |                       |
| `png_set_unknown_chunks()`   | [`spng_set_unknown_chunks()`](chunk.md#spng_set_unknown_chunks) |                       |
| `png_get_image_width()`      | [`spng_get_ihdr()`](chunk.md#spng_get_ihdr)                     |                       |
| `png_get_image_height()`     | [`spng_get_ihdr()`](chunk.md#spng_get_ihdr)                     |                       |
| `png_get_bit_depth()`        | [`spng_get_ihdr()`](chunk.md#spng_get_ihdr)                     |                       |
| `png_get_color_type()`       | [`spng_get_ihdr()`](chunk.md#spng_get_ihdr)                     |                       |
| `png_get_filter_type()`      | [`spng_get_ihdr()`](chunk.md#spng_get_ihdr)                     |                       |
| `png_get_interlace_type()`   | [`spng_get_ihdr()`](chunk.md#spng_get_ihdr)                     |                       |
| `png_get_compression_type()` | [`spng_get_ihdr()`](chunk.md#spng_get_ihdr)                     |                       |

# Decode

## Set source PNG

!!! note
    Push-style decoding is not supported in this release.

| libpng              | spng                                                      | Notes                   |
|---------------------|-----------------------------------------------------------|-------------------------|
| `png_set_read_fn()` | [`spng_set_png_stream()`](context.md#spng_set_png_stream) | Set PNG stream callback |
| `png_init_io()`     | [`spng_set_png_file()`](context.md#spng_set_png_file)     | Set PNG file (`FILE*`)  |
| N/A                 | [`spng_set_png_buffer()`](context.md#spng_set_png_buffer) | Set PNG buffer          |


## Decoder configuration

| libpng                         | spng                                                     | Notes                                                                   |
|--------------------------------|----------------------------------------------------------|-------------------------------------------------------------------------|
| `png_set_crc_action()`         | [`spng_set_crc_action()`](decode.md#spng_set_crc_action) | See [CRC actions](#crc-action-constants)                                |
| `png_set_interlace_handling()` | N/A                                                      | See [Progressive image decoding](decode.md#progressive-image-decoding). |
| `png_create_read_struct()`     | N/A                                                      | See [Contexts](#contexts)                                               |
| `png_destroy_read_struct()`    | N/A                                                      | See [Contexts](#contexts)                                               |

### CRC action constants

!!! note
    Errors are only signalled through return values, spng never calls `abort()`.

| libpng                 | spng               | Action - critical   | Action - ancillary  |
|------------------------|--------------------|---------------------|---------------------|
| `PNG_CRC_DEFAULT`      | N/A                | `SPNG_CRC_ERROR`    | `SPNG_CRC_USE`      |
| `PNG_CRC_ERROR_QUIT`   | `SPNG_CRC_ERROR`   | error               | error               |
| `PNG_CRC_WARN_DISCARD` | `SPNG_CRC_DISCARD` | (invalid)           | discard data        |
| `PNG_CRC_WARN_USE`     | N/A                | (no warning system) | (no warning system) |
| `PNG_CRC_QUIET_USE`    | `SPNG_CRC_USE`     | use data            | use data            |
| `PNG_CRC_NO_CHANGE`    | N/A                |                     |                     |


## Image formats and transforms

Currently the library only works with source and destination formats,
there are no transforms which change the output format based on the
source PNG's format or other metadata like the tRNS chunk.

Migrating from the traditional libpng API is straightforward if the desired output
format is clearly defined, nonetheless some transforms (such as [Adding transparency](#adding-transparency))
can be implemented on top of the existing API.

All [`spng_format`](context.md#spng_format)'s are explicit and host-endian (except for `SPNG_FMT_PNG` and `SPNG_FMT_RAW`),
PNG formats are converted to the destination `spng_format` when decoding or
converted from the `spng_format` to the destination PNG format when encoding.
When the destination format has an alpha channel and the source doesn't the alpha samples
are implicitly set to the fully opaque, maximum value.
The alpha channel is always [straight alpha](https://en.wikipedia.org/wiki/Alpha_compositing#Straight_versus_premultiplied),
premultiplied alpha is not supported.

!!! note
    Some source/destination combinations are not supported, check supported format, flags combinations for [decoding](decode.md#supported-format-flag-combinations) and [encoding](encode.md#supported-format-flag-combinations).

`SPNG_FMT_PNG` represents the PNG's format specified in the header (IHDR chunk) in host-endian (ie. little-endian on x86),
it is the equivalent of only calling `png_set_swap()` on little-endian when decoding or encoding.
When decoding the output will always be host-endian, when encoding it is assumed
the source image is host-endian.

`SPNG_FMT_RAW` is the same as `SPNG_FMT_PNG` but in big-endian,
it is the same as not doing any transformations when decoding or encoding with libpng.
The PNG standard only supports encoding of 16-bit images in big-endian, when used as the source
format the library will assume the source image is big-endian.
When decoding 16-bit images the output will always be big-endian.

Some decode flags may affect the final image but never the size or the layout of the samples within the pixels.

## Adding transparency

This can be achieved by specifiying an output format with an alpha channel such as `SPNG_FMT_RGBA8` and the `SPNG_DECODE_TRNS` decode flag.

```c
ret = spng_decode_image(ctx, out, len, SPNG_FMT_RGBA8, SPNG_DECODE_TRNS);
```

Note that using `SPNG_DECODE_TRNS` does not result in an error if the image does not have a tRNS chunk or
is not applicable for the PNG format/output format combination, in those cases the flag is ignored.

The `png_set_tRNS_to_alpha()` function applies a tranforms which
adds an alpha channel of the same bit depth if a tRNS chunk is present.
It also implicitly converts indexed color images to 8-bit RGB,
1/2/4-bit grayscale images to 8-bit grayscale and also adds an alpha channel
if there is a tRNS chunk.

This transform can be implemented by choosing the equivalent output format and using `SPNG_DECODE_TRNS` to apply transparency:

```c
int fmt = SPNG_FMT_PNG;
int decode_flags = SPNG_DECODE_TRNS;

if(ihdr.color_type == SPNG_COLOR_TYPE_INDEXED) fmt = SPNG_FMT_RGB8;
else if(ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE && ihdr.bit_depth < 8) fmt = SPNG_FMT_G8;

struct spng_trns trns = {0};
int have_trns = !spng_get_trns(ctx, &trns);

if(have_trns)
{
    if(ihdr.color_type == SPNG_COLOR_TYPE_TRUECOLOR)
    {
        if(ihdr.bit_depth == 16) fmt = SPNG_FMT_RGBA16;
        else fmt = SPNG_FMT_RGBA8;
    }
    else if(ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE)
    {
        if(ihdr.bit_depth == 16) fmt = SPNG_FMT_GA16;
        else fmt = SPNG_FMT_GA8;
    }
    else if(ihdr.color_type == SPNG_COLOR_TYPE_INDEXED)
    {
        fmt = SPNG_FMT_RGBA8;
    }
}
```

| libpng                             | spng               | Notes                                                     |
|------------------------------------|--------------------|-----------------------------------------------------------|
| `png_set_swap()`                   | N/A                | Byteswapping is done based on source / destination format |
| `png_set_expand_gray_1_2_4_to_8()` | N/A                | Use `SPNG_FMT_G8` when required                           |
| `png_set_tRNS_to_alpha()`          | N/A                | See [Adding transparency](#Adding-transparency)           |
| `png_set_palette_to_rgb()`         | N/A                | Use `SPNG_FMT_RGB8` when required                         |
| `png_set_gamma()`                  | `SPNG_DECODE_GAMA` | Screen gamma of 2.2 is assumed                            |
| `png_read_update_info()`           | N/A                | Not required                                              |
| `png_set_expand_16()`              | N/A                | Not supported                                             |
| `png_set_bgr()`                    | N/A                | Not supported                                             |
| `png_set_gray_to_rgb()`            | N/A                | Use `SPNG_FMT_RGB8` when required                         |
| `png_set_rgb_to_gray()`            | N/A                | Not supported                                             |
| `png_set_rgb_to_gray_fixed()`      | N/A                | Not supported                                             |
| `png_build_grayscale_palette()`    | N/A                | Not supported                                             |
| `png_set_alpha_mode()`             | N/A                | Not supported                                             |
| `png_set_alpha_mode_fixed()`       | N/A                | Not supported                                             |
| `png_set_strip_alpha()`            | N/A                | Not supported                                             |
| `png_set_swap_alpha()`             | N/A                | Not supported                                             |
| `png_set_invert_alpha()`           | N/A                | Not supported                                             |
| `png_set_filler()`                 | N/A                | Not supported                                             |
| `png_set_add_alpha()`              | N/A                | Not supported                                             |
| `png_set_swap_alpha()`             | N/A                | Not supported                                             |
| `png_set_packing()`                | N/A                | Not supported                                             |
| `png_set_packswap()`               | N/A                | Not supported                                             |
| `png_set_shift()`                  | N/A                | Not supported                                             |
| `png_set_invert_mono()`            | N/A                | Not supported                                             |
| `png_set_background()`             | N/A                | Not supported                                             |
| `png_set_background_fixed()`       | N/A                | Not supported                                             |
| `png_set_scale_16()`               | N/A                | Not supported                                             |
| `png_set_strip_16()`               | N/A                | Not supported                                             |

# Decoding images

See also: [Progressive image decoding](decode.md#progressive-image-decoding)

!!! note
    Row pointers (array of pointers) are not used.

| libpng              | spng                                                   | Notes                               |
|---------------------|--------------------------------------------------------|-------------------------------------|
| `png_read_row()`    | [`spng_decode_row()`](encode.md#spng_decode_row)       |                                     |
| `png_write_image()` | [`spng_encode_image()`](encode.md#spng_decode_image)   |                                     |
| `png_read_info()`   | [`spng_decode_chunks()`](decode.md#spng_decode_chunks) | Optional, chunks are read on-demand |
| `png_read_end()`    | [`spng_decode_chunks()`](decode.md#spng_decode_chunks) | Optional, chunks are read on-demand |


# Encode

## Set destination PNG

| libpng               | spng                                                                        | Notes                     |
|----------------------|-----------------------------------------------------------------------------|---------------------------|
| `png_set_write_fn()` | [`spng_set_png_stream()`](context.md#spng_set_png_stream)                   | Set PNG stream callback   |
| `png_init_io()`      | [`spng_set_png_file()`](decode.md#spng_set_png_file)                        | Set PNG file (`FILE*`)    |
| N/A                  | [`spng_set_option(ctx, SPNG_ENCODE_TO_BUFFER)`](context.md#spng_set_option) | Encode to internal buffer |


## Encoder configuration

| libpng                                   | spng                                                | Notes                                                        |
|------------------------------------------|-----------------------------------------------------|--------------------------------------------------------------|
| `png_create_write_struct()`              | N/A                                                 | See [Contexts](#contexts)                                    |
| `png_destroy_write_struct()`             | N/A                                                 | See [Contexts](#contexts)                                    |
| `png_set_keep_unknown_chunks()`          | [`spng_set_option()`](context.md#spng_set_option)   | `SPNG_KEEP_UNKNOWN_CHUNKS`                                   |
| `png_handle_as_unknown()`                | N/A                                                 | Not supported                                                |
| `png_set_unknown_chunk_location()`       | N/A                                                 | Not supported, set `location` field ahead of time            |
| `png_set_compression_level()`            | [`spng_set_option()`](context.md#spng_set_option)   | `SPNG_IMG_COMPRESSION_LEVEL`                                 |
| `png_set_compression_mem_level()`        | [`spng_set_option()`](context.md#spng_set_option)   | `SPNG_IMG_MEM_LEVEL`                                         |
| `png_set_compression_strategy()`         | [`spng_set_option()`](context.md#spng_set_option)   | `SPNG_IMG_COMPRESSION_STRATEGY`                              |
| `png_set_compression_window_bits()`      | [`spng_set_option()`](context.md#spng_set_option)   | `SPNG_IMG_WINDOW_BITS`                                       |
| `png_set_compression_method()`           | N/A                                                 | Not supported                                                |
| `png_set_text_compression_level()`       | [`spng_set_option()`](context.md#spng_set_option)   | `SPNG_TEXT_COMPRESSION_LEVEL`                                |
| `png_set_text_compression_mem_level()`   | [`spng_set_option()`](context.md#spng_set_option)   | `SPNG_TEXT_MEM_LEVEL`                                        |
| `png_set_text_compression_strategy()`    | [`spng_set_option()`](context.md#spng_set_option)   | `SPNG_TEXT_COMPRESSION_STRATEGY`                             |
| `png_set_text_compression_window_bits()` | [`spng_set_option()`](context.md#spng_set_option)   | `SPNG_TEXT_WINDOW_BITS`                                      |
| `png_set_text_compression_method()`      | N/A                                                 | Not supported                                                |
| `png_set_filter()`                       | [`spng_set_option()`](context.md#spng_set_option)   | `SPNG_FILTER_CHOICE` (see [Filter choices](#filter-choices)) |
| `png_set_chunk_cache_max()`              | [`spng_set_option()`](context.md#spng_set_option)   | `SPNG_CHUNK_COUNT_LIMIT`                                     |
| `png_set_chunk_malloc_max()`             | [`spng_set_chunk_limits()`](#spng_set_chunk_limits) | `chunk_size` argument                                        |
| `png_get_chunk_malloc_max()`             | [`spng_get_chunk_limits()`](#spng_get_chunk_limits) | `chunk_size` argument                                        |
| `png_set_quantize()`                     | N/A                                                 | Not supported                                                |


## Encoding images

See also: [Progressive image encoding](encode.md#progressive-image-encoding)

!!! note
    Row pointers (array of pointers) are not used.

| libpng                         | spng                                                   | Notes                                       |
|--------------------------------|--------------------------------------------------------|---------------------------------------------|
| `png_write_row()`              | [`spng_encode_row()`](encode.md#spng_encode_row)       |                                             |
| `png_write_image()`            | [`spng_encode_image()`](encode.md#spng_encode_image)   |                                             |
| `png_write_info_before_PLTE()` | N/A                                                    | Not supported                               |
| `png_write_info()`             | [`spng_encode_chunks()`](encode.md#spng_encode_chunks) | Optional, chunks are written on-demand      |
| `png_write_end()`              | [`spng_encode_chunks()`](encode.md#spng_encode_chunks) | Can be replaced with `SPNG_ENCODE_FINALIZE` |

# Common

## Chunk location constants

| libpng           | spng              |
|------------------|-------------------|
| `PNG_HAVE_IHDR`  | `SPNG_AFTER_IHDR` |
| `PNG_HAVE_PLTE`  | `SPNG_AFTER_PLTE` |
| `PNG_AFTER_IDAT` | `SPNG_AFTER_IDAT` |

## Color type constants

| libpng                      | spng                              |
|-----------------------------|-----------------------------------|
| `PNG_COLOR_TYPE_GRAY`       | `SPNG_COLOR_TYPE_GRAYSCALE`       |
| `PNG_COLOR_TYPE_PALETTE`    | `SPNG_COLOR_TYPE_INDEXED`         |
| `PNG_COLOR_TYPE_RGB`        | `SPNG_COLOR_TYPE_TRUECOLOR`       |
| `PNG_COLOR_TYPE_RGB_ALPHA`  | `SPNG_COLOR_TYPE_TRUECOLOR_ALPHA` |
| `PNG_COLOR_TYPE_GRAY_ALPHA` | `SPNG_COLOR_TYPE_GRAYSCALE_ALPHA` |
| `PNG_COLOR_TYPE_RGBA`       | `SPNG_COLOR_TYPE_TRUECOLOR_ALPHA` |
| `PNG_COLOR_TYPE_GA`         | `SPNG_COLOR_TYPE_GRAYSCALE_ALPHA` |


## Filter constants

### Filter values

Filter values defined by the standard.

| libpng                   | spng                  |
|--------------------------|-----------------------|
| `PNG_FILTER_VALUE_NONE`  | `SPNG_FILTER_NONE`    |
| `PNG_FILTER_VALUE_SUB `  | `SPNG_FILTER_SUB`     |
| `PNG_FILTER_VALUE_UP`    | `SPNG_FILTER_UP`      |
| `PNG_FILTER_VALUE_AVG`   | `SPNG_FILTER_AVERAGE` |
| `PNG_FILTER_VALUE_PAETH` | `SPNG_FILTER_PAETH`   |

### Filter choices

Filter choice flags for [`spng_set_option()`](context.md#spng_set_option) when used with `SPNG_FILTER_CHOICE`.

| libpng             | spng                       |
|--------------------|----------------------------|
| `PNG_NO_FILTERS`   | `SPNG_DISABLE_FILTERING`   |
| `PNG_FILTER_NONE`  | `SPNG_FILTER_CHOICE_NONE`  |
| `PNG_FILTER_SUB`   | `SPNG_FILTER_CHOICE_SUB`   |
| `PNG_FILTER_UP`    | `SPNG_FILTER_CHOICE_UP`    |
| `PNG_FILTER_AVG`   | `SPNG_FILTER_CHOICE_AVG`   |
| `PNG_FILTER_PAETH` | `SPNG_FILTER_CHOICE_PAETH` |
| `PNG_ALL_FILTERS`  | `SPNG_FILTER_CHOICE_ALL`   |


## Miscellaneous functions

| libpng                              | spng                                        | Notes                                             |
|-------------------------------------|---------------------------------------------|---------------------------------------------------|
| `png_create_info_struct()`          | N/A                                         | See [Contexts](#contexts)                         |
| `png_set_mem_fn()`                  | N/A                                         | Use [`spng_ctx_new2()`](context.md#spng_ctx_new2) |
| `png_get_mem_ptr()`                 | N/A                                         | Not supported                                     |
| `png_get_current_row_number()`      | [`spng_get_row_info()`](#spng_get_row_info) | `spng_row_info.row_num`                           |
| `png_get_current_pass_number()`     | [`spng_get_row_info()`](#spng_get_row_info) | `spng_row_info.pass`                              |
| `png_get_io_state()`                | N/A                                         | Not supported                                     |
| `png_set_option()`                  | `spng_set_option()`                         |                                                   |
| `png_set_invalid()`                 | N/A                                         | Not supported                                     |
| `png_set_sig_bytes()`               | N/A                                         | Not supported                                     |
| `png_sig_cmp()`                     | N/A                                         | Not supported                                     |
| `png_check_sig()`                   | N/A                                         | Not supported                                     |
| `png_get_compression_buffer_size()` | N/A                                         | Not supported                                     |
| `png_set_compression_buffer_size()` | N/A                                         | Not supported                                     |
| `png_jmpbuf()`                      | N/A                                         | Not supported                                     |
| `png_reset_zstream()`               | N/A                                         | Not supported                                     |
| `png_write_sig()`                   | N/A                                         | Not supported                                     |
| `png_write_chunk()`                 | N/A                                         | Not supported                                     |
| `png_write_chunk_start()`           | N/A                                         | Not supported                                     |
| `png_write_chunk_data()`            | N/A                                         | Not supported                                     |
| `png_write_chunk_end()`             | N/A                                         | Not supported                                     |