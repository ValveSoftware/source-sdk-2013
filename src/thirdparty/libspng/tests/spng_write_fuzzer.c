#include "../spng/spng.h"

#include <string.h>

struct buf_state
{
    const uint8_t *data;
    size_t bytes_left;
};

static int stream_write_fn(spng_ctx *ctx, void *user, void *data, size_t length)
{
    struct buf_state *state = (struct buf_state*)user;
    (void)ctx;

    if(length > state->bytes_left) return SPNG_IO_EOF;

    state->bytes_left -= length;

    if(state->data)
    {
        memcpy(data, state->data, length);
        state->data += length;
    }

    return 0;
}

#define copy_val(val) \
    if(size < sizeof(val)) goto err; \
    memcpy(&val, data, sizeof(val)); \
    data += sizeof(val); \
    size -= sizeof(val)

#ifdef __cplusplus
extern "C"
#endif
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    uint8_t params[4];

    if(size < sizeof(params)) return 0;

    memcpy(params, data, sizeof(params));

    size -= sizeof(params);
    data += sizeof(params);

    //int flags = params[0];
    int fmt = params[2] | (params[1] << 8);
    int stream = params[3] & 1;
    int progressive = params[3] & 2;
    //int file_stream = params[3] & 4;
    int get_buffer = params[3] & 8;

    int ret;
    size_t png_size = 0;
    void *png = NULL;

    struct buf_state state;
    unsigned char *img = NULL;
    size_t img_size;
    size_t pixel_bits = 0;

    spng_ctx *ctx = spng_ctx_new(SPNG_CTX_ENCODER);
    if(ctx == NULL) return 0;

    spng_set_image_limits(ctx, 200000, 200000);
    spng_set_chunk_limits(ctx, 4 * 1000 * 1000, 8 * 1000 * 1000);

    state.data = NULL;
    state.bytes_left = SIZE_MAX;

    spng_set_option(ctx, SPNG_IMG_COMPRESSION_LEVEL, 0);

    if(stream) ret = spng_set_png_stream(ctx, stream_write_fn, &state);
    else ret = spng_set_option(ctx, SPNG_ENCODE_TO_BUFFER, 1);

    if(ret) goto err;

    struct spng_ihdr ihdr;
    struct spng_plte plte;
    struct spng_trns trns;
    struct spng_chrm chrm;
    struct spng_chrm_int chrm_int;
    double gamma;
    struct spng_sbit sbit;
    uint8_t srgb_rendering_intent;
    struct spng_bkgd bkgd;
    struct spng_hist hist;
    struct spng_phys phys;
    struct spng_time time;

    copy_val(ihdr);
    copy_val(plte);
    copy_val(trns);
    copy_val(chrm);
    copy_val(chrm_int);
    copy_val(gamma);
    copy_val(sbit);
    copy_val(srgb_rendering_intent);
    copy_val(bkgd);
    copy_val(hist);
    copy_val(phys);
    copy_val(time);

    if(spng_set_ihdr(ctx, &ihdr)) goto err;

    spng_set_plte(ctx, &plte);
    spng_set_trns(ctx, &trns);
    spng_set_chrm(ctx, &chrm);
    spng_set_chrm_int(ctx, &chrm_int);
    spng_set_gama(ctx, gamma);
    spng_set_sbit(ctx, &sbit);
    spng_set_srgb(ctx, srgb_rendering_intent);
    spng_set_bkgd(ctx, &bkgd);
    spng_set_hist(ctx, &hist);
    spng_set_phys(ctx, &phys);
    spng_set_time(ctx, &time);

    switch(ihdr.color_type)
    {
        case SPNG_COLOR_TYPE_GRAYSCALE:
        case SPNG_COLOR_TYPE_INDEXED:
            pixel_bits = ihdr.bit_depth;
            break;
        case SPNG_COLOR_TYPE_TRUECOLOR:
            pixel_bits = ihdr.bit_depth * 3;
            break;
        case SPNG_COLOR_TYPE_GRAYSCALE_ALPHA:
            pixel_bits = ihdr.bit_depth * 2;
            break;
        case SPNG_COLOR_TYPE_TRUECOLOR_ALPHA:
            pixel_bits = ihdr.bit_depth * 4;
            break;
        default: goto err;
    }

    img_size = ihdr.width * pixel_bits + 7;

    img_size /= 8;

    img_size *= ihdr.height;

    if(img_size > size) goto err;

    if(img_size > 80000000) goto err;

    img = (unsigned char*)data;

    if(progressive)
    {
        if(spng_encode_image(ctx, NULL, 0, fmt, SPNG_ENCODE_PROGRESSIVE | SPNG_ENCODE_FINALIZE)) goto err;

        size_t ioffset, img_width = img_size / ihdr.height;
        struct spng_row_info ri;

        do
        {
            if(spng_get_row_info(ctx, &ri)) break;

            ioffset = ri.row_num * img_width;

        }while(!spng_encode_row(ctx, img + ioffset, img_size));
    }
    else if(spng_encode_image(ctx, img, img_size, fmt, SPNG_ENCODE_FINALIZE)) goto err;

    if(get_buffer)
    {
        png = spng_get_png_buffer(ctx, &png_size, &ret);

        // These are potential vulnerabilities
        if(png && !png_size) return 1;
        if(!png && png_size) return 1;
        if(ret && (png || png_size)) return 1;
    }

err:
    spng_ctx_free(ctx);
    free(png);

    return 0;
}
