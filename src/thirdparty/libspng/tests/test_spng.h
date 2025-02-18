#ifndef TEST_SPNG_H
#define TEST_SPNG_H

#include "spngt_common.h"

#include <spng.h>
#include <string.h>

int spng_get_trns_fmt(spng_ctx *ctx, int *fmt)
{
   if(ctx == NULL || fmt == NULL) return 1;

    struct spng_trns trns;
    struct spng_ihdr ihdr;

    if(spng_get_ihdr(ctx, &ihdr)) return 1;

    if(!spng_get_trns(ctx, &trns))
    {
        if(ihdr.color_type == SPNG_COLOR_TYPE_TRUECOLOR)
        {
            if(ihdr.bit_depth == 16) *fmt = SPNG_FMT_RGBA16;
            else *fmt = SPNG_FMT_RGBA8;
        }
        else if(ihdr.color_type == SPNG_COLOR_TYPE_INDEXED)
        {
            *fmt = SPNG_FMT_RGBA8;
        }
        else if(ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE)
        {
            if(ihdr.bit_depth == 16) *fmt = SPNG_FMT_GA16;
            else *fmt = SPNG_FMT_GA8;
        }
    }
    else return 1;

    return 0;
}

spng_ctx *init_spng(spngt_test_case *test_case, struct spng_ihdr *ihdr)
{
    int r = 0;
    spng_ctx *ctx = spng_ctx_new(0);

    if(ctx == NULL)
    {
        printf("spng_ctx_new() failed\n");
        return NULL;
    }

    if(test_case->source.type == SPNGT_SRC_FILE) r = spng_set_png_file(ctx, test_case->source.file);
    else if(test_case->source.type == SPNGT_SRC_BUFFER) r = spng_set_png_buffer(ctx, test_case->source.buffer, test_case->source.png_size);

    if(r)
    {
        printf("spng_set_png_file/buffer() error: %s\n", spng_strerror(r));
        goto err;
    }

    r = spng_set_option(ctx, SPNG_KEEP_UNKNOWN_CHUNKS, 1);

    if(r)
    {
        printf("spng_set_option() error: %s\n", spng_strerror(r));
        goto err;
    }

    r = spng_set_image_limits(ctx, 16000, 16000);

    if(r)
    {
        printf("spng_set_image_limits() error: %s\n", spng_strerror(r));
        goto err;
    }

    r = spng_set_chunk_limits(ctx, 66 * 1000 * 1000, 66 * 1000* 1000);

    if(r)
    {
        printf("spng_set_chunk_limits() error: %s\n", spng_strerror(r));
        goto err;
    }

    struct spng_ihdr tmp;
    if(!ihdr) ihdr = &tmp;

    r = spng_get_ihdr(ctx, ihdr);

    if(r)
    {
        printf("spng_get_ihdr() error: %s\n", spng_strerror(r));
        goto err;
    }

    return ctx;

err:
    spng_ctx_free(ctx);
    return NULL;
}

unsigned char *getimage_spng(spng_ctx *ctx, size_t *out_size, int fmt, int flags)
{
    int r;
    size_t siz, out_width;
    unsigned char *out = NULL;
    struct spng_ihdr ihdr;
    struct spng_row_info row_info;

    r = spng_get_ihdr(ctx, &ihdr);

    if(r)
    {
        printf("spng_get_ihdr() error: %s\n", spng_strerror(r));
        goto err;
    }

    r = spng_decode_chunks(ctx);

    if(r)
    {
        printf("spng_decode_chunks() error: %s\n", spng_strerror(r));
        goto err;
    }

    if(fmt == SPNGT_FMT_VIPS)
    {
        fmt = SPNG_FMT_PNG;
        if(ihdr.color_type == SPNG_COLOR_TYPE_INDEXED) fmt = SPNG_FMT_RGB8;
        else if(ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE && ihdr.bit_depth < 8) fmt = SPNG_FMT_G8;

        spng_get_trns_fmt(ctx, &fmt);
        /*printf("VIPS format: %s\n", fmt_str(fmt));*/
    }

    r = spng_decoded_image_size(ctx, fmt, &siz);
    if(r)
    {
        printf("spng_decoded_image_size() error: %s\n", spng_strerror(r));
        goto err;
    }


    *out_size = siz;
    out_width = siz / ihdr.height;

    /* Neither library does zero-padding for <8-bit images,
    but we want the images to be bit-identical for memcmp() */
    out = calloc(1, siz);
    if(out == NULL) goto err;

    r = spng_decode_image(ctx, NULL, 0, fmt, flags | SPNG_DECODE_PROGRESSIVE);

    if(r)
    {
        printf("progressive spng_decode_image() error: %s\n", spng_strerror(r));
        goto err;
    }

    do
    {
        r = spng_get_row_info(ctx, &row_info);
        if(r) break;

        r = spng_decode_row(ctx, out + row_info.row_num * out_width, out_width);
    }while (!r);

    if(r != SPNG_EOI)
    {
        printf("progressive decode error: %s\n", spng_strerror(r));
        goto err;
    }

    r = spng_get_time(ctx, NULL);

    if(r == SPNG_ECHUNKAVAIL || r == SPNG_EINVAL) r = 0;

    if(r)
    {
        printf("error at end of image: %s\n", spng_strerror(r));
        goto err;
    }

    return out;

err:
    if(out != NULL) free(out);
    return NULL;
}

#endif /* TEST_SPNG_H */
