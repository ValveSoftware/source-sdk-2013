#ifndef TEST_PNG_H
#define TEST_PNG_H

#if (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) || defined(__BIG_ENDIAN__)
    #define SPNGT_BIG_ENDIAN
#else
    #define SPNGT_LITTLE_ENDIAN
#endif

#include <png.h>

#include "spngt_common.h"

#include <stdlib.h>
#include <string.h>

struct buf_state
{
    unsigned char *data;
    size_t bytes_left;
};

static struct buf_state state;

void libpng_read_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
    struct buf_state *state = png_get_io_ptr(png_ptr);

#if defined(SPNGT_STREAM_READ_INFO)
    printf("libpng bytes read: %lu\n", length);
#endif

    if(length > state->bytes_left)
    {
        png_error(png_ptr, "read_fn error");
    }

    memcpy(data, state->data, length);
    state->bytes_left -= length;
    state->data += length;
}

png_structp init_libpng(spngt_test_case *test_case, png_infop *iptr)
{
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(png_ptr == NULL)
    {
        printf("libpng init failed\n");
        return NULL;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if(info_ptr == NULL)
    {
        printf("png_create_info_struct failed\n");
        return NULL;
    }

    if(test_case->source.type == SPNGT_SRC_FILE) png_init_io(png_ptr, test_case->source.file);
    else if(test_case->source.type == SPNGT_SRC_BUFFER)
    {
        state.data = test_case->source.buffer;
        state.bytes_left = test_case->source.png_size;
        png_set_read_fn(png_ptr, &state, libpng_read_fn);
    }

    if(setjmp(png_jmpbuf(png_ptr)))
    {
        printf("libpng error\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    png_set_keep_unknown_chunks(png_ptr, PNG_HANDLE_CHUNK_ALWAYS, NULL, 0);

    png_read_info(png_ptr, info_ptr);

    *iptr = info_ptr;

    return png_ptr;
}

unsigned char *getimage_libpng(png_structp png_ptr, png_infop info_ptr, size_t *out_size, int fmt, int flags)
{
    unsigned char *volatile image = NULL;
    png_bytep *volatile row_pointers = NULL;

    if(setjmp(png_jmpbuf(png_ptr)))
    {
        printf("libpng error\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        if(image != NULL) free(image);
        if(row_pointers != NULL) free(row_pointers);
        return NULL;
    }

    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type, compression_type;
    int filter_method;

    if(!png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
                     &color_type, &interlace_type, &compression_type, &filter_method))
    {
        printf("png_get_IHDR failed\n");
        return NULL;
    }

    if(flags & SPNG_DECODE_GAMMA)
    {
        double gamma;
        if(png_get_gAMA(png_ptr, info_ptr, &gamma))
            png_set_gamma(png_ptr, 2.2, gamma);
    }

    if(fmt == SPNG_FMT_RGBA16)
    {
        png_set_gray_to_rgb(png_ptr);

        png_set_filler(png_ptr, 0xFFFF, PNG_FILLER_AFTER);

        /* png_set_palette_to_rgb() + png_set_tRNS_to_alpha() */
        png_set_expand_16(png_ptr);
    }
    else if(fmt == SPNG_FMT_RGBA8)
    {
        png_set_gray_to_rgb(png_ptr);

        png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

        /* png_set_palette_to_rgb() + png_set_expand_gray_1_2_4_to_8() + png_set_tRNS_to_alpha() */
        png_set_expand(png_ptr);

        png_set_strip_16(png_ptr);
    }
    else if(fmt == SPNG_FMT_RGB8)
    {
        png_set_gray_to_rgb(png_ptr);

        png_set_strip_alpha(png_ptr);

        png_set_strip_16(png_ptr);
    }
    else if(fmt == SPNG_FMT_G8) /* assumes only <=8-bit grayscale images */
    {/* TODO: support all input formats */
        png_set_expand_gray_1_2_4_to_8(png_ptr);
    }
    else if(fmt == SPNG_FMT_GA16) /* assumes only 16-bit grayscale images */
    {/* TODO: support all input formats */
        if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) && (flags & SPNG_DECODE_TRNS))
        {
            png_set_tRNS_to_alpha(png_ptr);
        }
        else
        {
            if(bit_depth == 16) png_set_filler(png_ptr, 0xFFFF, PNG_FILLER_AFTER);
        }
    }
    else if(fmt == SPNG_FMT_GA8) /* assumes only <=8-bit grayscale images */
    {/* TODO: support all input formats */
        if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) && (flags & SPNG_DECODE_TRNS))
        {
            png_set_tRNS_to_alpha(png_ptr);
        }
        else
        {
            png_set_expand_gray_1_2_4_to_8(png_ptr);
            png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
        }
    }
    else if(fmt == SPNGT_FMT_VIPS)
    {
        if(color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);

        if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);

        if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);

#if defined(SPNGT_LITTLE_ENDIAN)
        png_set_swap(png_ptr);
#endif
    }

#if defined(SPNGT_LITTLE_ENDIAN) /* we want host-endian values unless it's SPNG_FMT_RAW */
    if(fmt != SPNG_FMT_RAW) png_set_swap(png_ptr);
#endif

    png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);

    size_t image_size = height * rowbytes;
    memcpy(out_size, &image_size, sizeof(size_t));

    /* Neither library does zero-padding for <8-bit images,
       but we want the images to be bit-identical for memcmp() */
    image = calloc(1, image_size);
    if(image == NULL)
    {
        printf("libpng: malloc() failed\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    row_pointers = malloc(height * sizeof(png_bytep));
    if(row_pointers == NULL)
    {
        printf("libpng: malloc() failed\n");
        free(image);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    uint32_t k;
    for(k=0; k < height; k++)
    {
        row_pointers[k] = image + k * rowbytes;
    }

    png_read_image(png_ptr, row_pointers);

    png_read_end(png_ptr, info_ptr);

    free(row_pointers);

    return image;
}

#endif /* TEST_PNG_H */
