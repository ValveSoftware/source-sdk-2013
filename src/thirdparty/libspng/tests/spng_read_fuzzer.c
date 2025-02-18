#define SPNG_UNTESTED
#include "../spng/spng.h"

#include <string.h>

struct buf_state
{
    const uint8_t *data;
    size_t bytes_left;
};

static int buffer_read_fn(spng_ctx *ctx, void *user, void *dest, size_t length)
{
    struct buf_state *state = (struct buf_state*)user;
    (void)ctx;

    if(length > state->bytes_left) return SPNG_IO_EOF;

    memcpy(dest, state->data, length);

    state->bytes_left -= length;
    state->data += length;

    return 0;
}

#ifdef __cplusplus
extern "C"
#endif
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if(size < 4) return 0;

    int flags = data[size - 1];
    int fmt = data[size - 3] | (data[size - 2] << 8);
    int stream = data[size - 4] & 1;
    int progressive = data[size - 4] & 2;
    int file_stream = data[size - 4] & 4;
    int discard = data[size - 4] & 8;

    size -= 4;

    int ret;
    unsigned char *out = NULL;
    FILE *file = NULL;

    spng_ctx *ctx = spng_ctx_new(SPNG_CTX_IGNORE_ADLER32);
    if(ctx == NULL) return 0;

    struct spng_ihdr ihdr;
    struct spng_plte plte;
    struct spng_trns trns;
    struct spng_chrm chrm;
    struct spng_chrm_int chrm_int;
    double gamma;
    struct spng_iccp iccp;
    struct spng_sbit sbit;
    uint8_t srgb_rendering_intent;
    struct spng_text text[4] = {0};
    struct spng_bkgd bkgd;
    struct spng_hist hist;
    struct spng_phys phys;
    struct spng_splt splt[4] = {0};
    struct spng_time time;
    struct spng_unknown_chunk chunks[4] = {0};
    uint32_t n_text = 4, n_splt = 4, n_chunks = 4;

    struct buf_state state;
    state.data = data;
    state.bytes_left = size;

    if(stream)
    {
        if(file_stream)
        {
#if defined(SPNGT_HAVE_FMEMOPEN)
            file = fmemopen((void*)data, size, "rb");

            if(file == NULL) goto err;
#endif
            ret = spng_set_png_file(ctx, file);
        }
        else ret = spng_set_png_stream(ctx, buffer_read_fn, &state);
    }
    else ret = spng_set_png_buffer(ctx, (void*)data, size);

    if(ret) goto err;

    spng_set_image_limits(ctx, 200000, 200000);

    spng_set_chunk_limits(ctx, 4 * 1000 * 1000, 8 * 1000 * 1000);

    spng_set_crc_action(ctx, SPNG_CRC_USE, discard ? SPNG_CRC_DISCARD : SPNG_CRC_USE);

    size_t out_size;
    if(spng_decoded_image_size(ctx, fmt, &out_size)) goto err;

    if(out_size > 80000000) goto err;

    out = (unsigned char*)malloc(out_size);
    if(out == NULL) goto err;

    spng_get_ihdr(ctx, &ihdr);
    spng_get_plte(ctx, &plte);
    spng_get_trns(ctx, &trns);
    spng_get_chrm(ctx, &chrm);
    spng_get_chrm_int(ctx, &chrm_int);
    spng_get_gama(ctx, &gamma);
    spng_get_iccp(ctx, &iccp);
    spng_get_sbit(ctx, &sbit);
    spng_get_srgb(ctx, &srgb_rendering_intent);

    if(!spng_get_text(ctx, text, &n_text))
    {/* Up to 4 entries were read, get the actual count */
        spng_get_text(ctx, NULL, &n_text);

        uint32_t i;
        for(i=0; i < n_text; i++)
        {/* All strings should be non-NULL */
            if(text[i].text == NULL ||
               text[i].keyword[0] == '\0' ||
               text[i].language_tag == NULL ||
               text[i].translated_keyword == NULL ||
               memchr(text[i].keyword, 0, 80) == NULL)
            {
                spng_ctx_free(ctx);
                free(out);
                return 1;
            }
            /* This shouldn't cause issues either */
            text[i].length = strlen(text[i].text);
        }
    }

    spng_get_bkgd(ctx, &bkgd);
    spng_get_hist(ctx, &hist);
    spng_get_phys(ctx, &phys);

    if(!spng_get_splt(ctx, splt, &n_splt))
    {/* Up to 4 entries were read, get the actual count */
        spng_get_splt(ctx, NULL, &n_splt);

        uint32_t i;
        for(i=0; i < n_splt; i++)
        {
            if(splt[i].name[0] == '\0' ||
               splt[i].entries == NULL ||
               memchr(splt[i].name, 0, 80) == NULL)
            {
                spng_ctx_free(ctx);
                free(out);
                return 1;
            }
        }
    }

    if(!spng_get_unknown_chunks(ctx, chunks, &n_chunks))
    {
        spng_get_unknown_chunks(ctx, NULL, &n_chunks);

        uint32_t i;
        for(i=0; i < n_chunks; i++)
        {
            if( (chunks[i].length && !chunks[i].data) ||
                (!chunks[i].length && chunks[i].data) )
            {
                spng_ctx_free(ctx);
                free(out);
                return 1;
            }
        }
    }

    if(progressive)
    {
        if(spng_decode_image(ctx, NULL, 0, fmt, flags | SPNG_DECODE_PROGRESSIVE)) goto err;

        size_t ioffset, out_width = out_size / ihdr.height;
        struct spng_row_info ri;

        do
        {
            if(spng_get_row_info(ctx, &ri)) break;

            ioffset = ri.row_num * out_width;

        }while(!spng_decode_row(ctx, out + ioffset, out_size));
    }
    else if(spng_decode_image(ctx, out, out_size, fmt, flags)) goto err;

    spng_get_time(ctx, &time);

err:
    spng_ctx_free(ctx);
    free(out);
    if(file != NULL) fclose(file);

    return 0;
}
