#include "spngt_common.h"

#include "test_spng.h"
#include "test_png.h"

#include <errno.h>

static int n_test_cases, actual_count;
static struct spngt_test_case test_cases[100];

static int extended_tests(FILE *file, int fmt);

const char* fmt_str(int fmt)
{
    switch(fmt)
    {
        case SPNG_FMT_RGBA8: return "RGBA8";
        case SPNG_FMT_RGBA16: return "RGBA16";
        case SPNG_FMT_RGB8: return "RGB8";
        case SPNG_FMT_GA8: return "GA8";
        case SPNG_FMT_GA16: return "GA16";
        case SPNG_FMT_G8: return "G8";
        case SPNG_FMT_PNG: return "PNG";
        case SPNG_FMT_RAW: return "RAW";
        case SPNGT_FMT_VIPS: return "VIPS";
        default: return "   ";
    }
}

static void print_test_args(struct spngt_test_case *test_case)
{
    char *type;

    if(test_case->test_flags & SPNGT_ENCODE_ROUNDTRIP) type = "Encode";
    else type = "Decode";

    printf("%s and compare %s", type, fmt_str(test_case->fmt));

    char pad_str[] = "      ";
    pad_str[sizeof(pad_str) - strlen(fmt_str(test_case->fmt))] = '\0';

    printf(",%sFLAGS: ", pad_str);

    if(!test_case->flags && !test_case->test_flags) printf("(NONE)");

    if(test_case->flags & SPNG_DECODE_TRNS) printf("TRNS ");
    if(test_case->flags & SPNG_DECODE_GAMMA) printf("GAMMA ");

    if(test_case->test_flags & SPNGT_COMPARE_CHUNKS) printf("COMPARE_CHUNKS ");

    if(test_case->test_flags & SPNGT_EXTENDED_TESTS) printf("EXTENDED ");

    if(test_case->test_flags & SPNGT_SKIP) printf("[SKIPPED]");

    printf("\n");

    fflush(stdout);
}

static void add_test_case(int fmt, int flags, int test_flags)
{
    int n = n_test_cases;

    test_cases[n].fmt = fmt;
    test_cases[n].flags = flags;
    test_cases[n_test_cases++].test_flags = test_flags;

    if( !(test_flags & SPNGT_SKIP) ) actual_count++;
}

/* Returns 1 on different images with, allows a small difference if gamma corrected */
static int compare_images(const struct spng_ihdr *ihdr,
                          int fmt,
                          int flags,
                          const unsigned char *img_spng,
                          const unsigned char *img_png,
                          size_t img_size)
{
    uint32_t w, h;
    uint32_t x, y;
    uint32_t max_diff=0, diff_div = 50; /* allow up to ~2% difference for each channel */
    uint8_t have_alpha = 1;
    uint8_t alpha_mismatch = 0;
    uint8_t pixel_diff = 0;
    uint8_t bytes_per_pixel = 4; /* SPNG_FMT_RGBA8 */
    const uint8_t samples_per_byte = 8 / ihdr->bit_depth;
    const uint8_t mask = (uint16_t)(1 << ihdr->bit_depth) - 1;
    const uint8_t initial_shift = 8 - ihdr->bit_depth;
    uint8_t shift_amount = initial_shift;
    size_t row_width = img_size / ihdr->height;
    size_t px_ofs = 0;
    unsigned channels;

    uint32_t red_diff = 0, green_diff = 0, blue_diff = 0, sample_diff = 0;
    uint16_t spng_red = 0, spng_green = 0, spng_blue = 0, spng_alpha = 0, spng_sample = 0;
    uint16_t png_red = 0, png_green = 0, png_blue = 0, png_alpha = 0, png_sample = 0;

    w = ihdr->width;
    h = ihdr->height;

    if(fmt & (SPNG_FMT_PNG | SPNG_FMT_RAW))
    {
        if(ihdr->color_type == SPNG_COLOR_TYPE_TRUECOLOR)
        {
            if(ihdr->bit_depth == 8) fmt = SPNG_FMT_RGB8;
            else fmt = SPNG_FMT_RGB16;
        }
        else if(ihdr->color_type == SPNG_COLOR_TYPE_TRUECOLOR_ALPHA)
        {
            if(ihdr->bit_depth == 8) fmt = SPNG_FMT_RGBA8;
            else fmt = SPNG_FMT_RGBA16;
        }
        else
        {/* gray 1,2,4,8,16 bits, indexed 1,2,4,8 or gray alpha 8,16  */
            channels = 1; /* grayscale or indexed color */
            if(ihdr->color_type == SPNG_COLOR_TYPE_GRAYSCALE_ALPHA) channels = 2;
            else have_alpha = 0;

            if(ihdr->bit_depth < 8) bytes_per_pixel = 1;
            else bytes_per_pixel = channels * (ihdr->bit_depth / 8);

            if(ihdr->color_type == SPNG_COLOR_TYPE_INDEXED) flags &= ~SPNG_DECODE_GAMMA;
        }
    }

    if(fmt == SPNG_FMT_RGBA8)
    {
        bytes_per_pixel = 4;
        max_diff = 256 / diff_div;
    }
    else if(fmt == SPNG_FMT_RGBA16)
    {
        bytes_per_pixel = 8;
        max_diff = 65536 / diff_div;
    }
    else if(fmt == SPNG_FMT_RGB8)
    {
        bytes_per_pixel = 3;
        have_alpha = 0;
        max_diff = 256 / diff_div;
    }
    else if(fmt == SPNG_FMT_RGB16)
    {
        bytes_per_pixel = 6;
        have_alpha = 0;
        max_diff = 65536 / diff_div;
    }
    else if(fmt == SPNG_FMT_GA8)
    {
        bytes_per_pixel = 2;
        have_alpha = 1;
        max_diff = 256 / diff_div;
    }
    else if(fmt == SPNG_FMT_G8)
    {
        bytes_per_pixel = 1;
        have_alpha = 0;
        max_diff = 256 / diff_div;
    }

    for(y=0; y < h; y++)
    {
        for(x=0; x < w; x++)
        {
            if(fmt & (SPNG_FMT_PNG | SPNG_FMT_RAW) && ihdr->bit_depth < 8)
                px_ofs = (y * row_width) + x / samples_per_byte;
            else
                px_ofs = (x + (y * w)) * bytes_per_pixel;

            if(fmt & (SPNG_FMT_RGBA16 | SPNG_FMT_RGB16))
            {
                uint16_t s_red, s_green, s_blue, s_alpha;
                uint16_t p_red, p_green, p_blue, p_alpha;

                memcpy(&s_red, img_spng + px_ofs, 2);
                memcpy(&s_green, img_spng + px_ofs + 2, 2);
                memcpy(&s_blue, img_spng + px_ofs + 4, 2);

                memcpy(&p_red, img_png + px_ofs, 2);
                memcpy(&p_green, img_png + px_ofs + 2, 2);
                memcpy(&p_blue, img_png + px_ofs + 4, 2);

                if(have_alpha)
                {
                    memcpy(&s_alpha, img_spng + px_ofs + 6, 2);
                    memcpy(&p_alpha, img_png + px_ofs + 6, 2);
                    spng_alpha = s_alpha;
                    png_alpha = p_alpha;
                }

                spng_red = s_red;
                spng_green = s_green;
                spng_blue = s_blue;

                png_red = p_red;
                png_green = p_green;
                png_blue = p_blue;
            }
            else if(fmt & (SPNG_FMT_RGBA8 | SPNG_FMT_RGB8))
            {
                uint8_t s_red, s_green, s_blue, s_alpha;
                uint8_t p_red, p_green, p_blue, p_alpha;

                memcpy(&s_red, img_spng + px_ofs, 1);
                memcpy(&s_green, img_spng + px_ofs + 1, 1);
                memcpy(&s_blue, img_spng + px_ofs + 2, 1);

                memcpy(&p_red, img_png + px_ofs, 1);
                memcpy(&p_green, img_png + px_ofs + 1, 1);
                memcpy(&p_blue, img_png + px_ofs + 2, 1);

                if(have_alpha)
                {
                    memcpy(&s_alpha, img_spng + px_ofs + 3, 1);
                    memcpy(&p_alpha, img_png + px_ofs + 3, 1);
                    spng_alpha = s_alpha;
                    png_alpha = p_alpha;
                }

                spng_red = s_red;
                spng_green = s_green;
                spng_blue = s_blue;

                png_red = p_red;
                png_green = p_green;
                png_blue = p_blue;
            }
            else if(fmt & (SPNG_FMT_PNG | SPNG_FMT_RAW | SPNG_FMT_G8 | SPNG_FMT_GA8 | SPNG_FMT_GA16))
            {
                if(ihdr->bit_depth <= 8) /* gray 1-8, gray-alpha 8, indexed 1-8 */
                {
                    uint8_t s_alpha, s_sample;
                    uint8_t p_alpha, p_sample;

                    memcpy(&s_sample, img_spng + px_ofs, 1);
                    memcpy(&p_sample, img_png + px_ofs, 1);

                    if(shift_amount > 7) shift_amount = initial_shift;

                    s_sample = (s_sample >> shift_amount) & mask;
                    p_sample = (p_sample >> shift_amount) & mask;

                    shift_amount -= ihdr->bit_depth;

                    spng_sample = s_sample;
                    png_sample = p_sample;

                   if(have_alpha)
                    {
                        memcpy(&s_alpha, img_spng + px_ofs + 1, 1);
                        memcpy(&p_alpha, img_png + px_ofs + 1, 1);
                        spng_alpha = s_alpha;
                        png_alpha = p_alpha;
                    }
                }
                else /* gray 16, gray-alpha 16 */
                {
                    uint16_t s_alpha, s_sample;
                    uint16_t p_alpha, p_sample;

                    memcpy(&s_sample, img_spng + px_ofs, 2);
                    memcpy(&p_sample, img_png + px_ofs, 2);

                    spng_sample = s_sample;
                    png_sample = p_sample;

                    if(have_alpha)
                    {
                        memcpy(&s_alpha, img_spng + px_ofs + 2, 2);
                        memcpy(&p_alpha, img_png + px_ofs + 2, 2);
                        spng_alpha = s_alpha;
                        png_alpha = p_alpha;
                    }
                }
            }

            if(spng_red != png_red || spng_green != png_green || spng_blue != png_blue || spng_sample != png_sample)
            {
                if(flags & SPNG_DECODE_GAMMA)
                {
                    red_diff = abs(spng_red - png_red);
                    green_diff = abs(spng_green - png_green);
                    blue_diff = abs(spng_blue - png_blue);
                    sample_diff = abs(spng_sample - png_sample);

                    if(red_diff > max_diff || green_diff > max_diff || blue_diff > max_diff)
                    {
                        printf("invalid gamma correction at x: %" PRIu32 " y: %" PRIu32 ", "
                               "spng: %" PRIu16 " %" PRIu16 " %" PRIu16 " "
                               "png: %" PRIu16 " %" PRIu16 " %" PRIu16 "\n",
                               x, y,
                               spng_red, spng_green, spng_blue,
                               png_red, png_green, png_blue);
                        pixel_diff = 1;
                    }
                    else if(sample_diff > max_diff)
                    {
                        printf("invalid gamma correction at x: %" PRIu32 " y: %" PRIu32 ", "
                               "spng: %" PRIu16 " png: %" PRIu16 "\n", x, y, spng_sample, png_sample);
                        pixel_diff = 1;
                    }
                }
                else
                {
                    if(spng_sample != png_sample)
                    {
                        char *issue_str = "";
                        if(ihdr->color_type == SPNG_COLOR_TYPE_INDEXED) issue_str = "index mismatch";
                        else issue_str = "grayscale difference";

                        printf("%s at x: %u y: %u spng: %u png: %u\n",
                               issue_str, x, y, spng_sample, png_sample);
                    }
                    else
                    {
                        printf("color difference at x: %" PRIu32 " y: %" PRIu32 ", "
                               "spng: %" PRIu16 " %" PRIu16 " %" PRIu16 " "
                               "png: %" PRIu16 " %" PRIu16 " %" PRIu16 "\n",
                               x, y,
                               spng_red, spng_green, spng_blue,
                               png_red, png_green, png_blue);
                    }

                    pixel_diff = 1;
                }
            }

            if(have_alpha && spng_alpha != png_alpha)
            {
                printf("alpha mismatch at x:%" PRIu32 " y:%" PRIu32 ", "
                       "spng: %" PRIu16 " png: %" PRIu16 "\n",
                       x, y, spng_alpha, png_alpha);
                alpha_mismatch = 1;
            }
        }
    }

    if(alpha_mismatch || pixel_diff) return 1;

    return 0;
}

static void print_chunks(spngt_chunk_bitfield chunks)
{
    spngt_chunk_bitfield none = { 0 };

    if(!memcmp(&none, &chunks, sizeof(spngt_chunk_bitfield))) printf(" (none)");

    if(chunks.plte) printf(" PLTE");
    if(chunks.trns) printf(" tRNS");
    if(chunks.chrm) printf(" cHRM");
    if(chunks.gama) printf(" gAMA");
    if(chunks.iccp) printf(" iCCP");
    if(chunks.sbit) printf(" sBIT");
    if(chunks.srgb) printf(" sRGB");
    if(chunks.text) printf(" tEXt");
    if(chunks.ztxt) printf(" zTXt");
    if(chunks.itxt) printf(" iTXt");
    if(chunks.bkgd) printf(" bKGD");
    if(chunks.hist) printf(" hIST");
    if(chunks.phys) printf(" pHYs");
    if(chunks.splt) printf(" sPLT");
    if(chunks.time) printf(" tIME");
    if(chunks.offs) printf(" oFFs");
    if(chunks.exif) printf(" eXIF");
    if(chunks.unknown) printf(" (unknown)");
}

static void free_chunks(spngt_chunk_data *spng)
{
    free(spng->splt);
    free(spng->text);
    free(spng->chunks);

    spng->splt = NULL;
    spng->text = NULL;
    spng->chunks = NULL;
}

static int get_chunks(spng_ctx *ctx, spngt_chunk_data *spng)
{
    int ret = spng_get_ihdr(ctx, &spng->ihdr);

    if(!ret) spng->have.ihdr = 1;
    else return ret;

    ret = spng_get_plte(ctx, &spng->plte);

    if(!ret)
    {
        spng->n_plte_entries = spng->plte.n_entries;
        spng->have.plte = 1;
    }
    else if(ret == SPNG_ECHUNKAVAIL) ret = 0;
    else return ret;

    if(!spng_get_trns(ctx, &spng->trns)) spng->have.trns = 1;
    if(!spng_get_chrm(ctx, &spng->chrm)) spng->have.chrm = 1;
    if(spng->have.chrm) spng_get_chrm_int(ctx, &spng->chrm_int);
    if(!spng_get_gama(ctx, &spng->gamma)) spng->have.gama = 1;
    if(spng->have.gama) spng_get_gama_int(ctx, &spng->gamma_int);
    if(!spng_get_iccp(ctx, &spng->iccp)) spng->have.iccp = 1;
    if(!spng_get_sbit(ctx, &spng->sbit)) spng->have.sbit = 1;
    if(!spng_get_srgb(ctx, &spng->srgb_rendering_intent)) spng->have.srgb = 1;
    if(!spng_get_text(ctx, NULL, &spng->n_text)) spng->have.text = 1;
    if(!spng_get_bkgd(ctx, &spng->bkgd)) spng->have.bkgd = 1;
    if(!spng_get_hist(ctx, &spng->hist)) spng->have.hist = 1;
    if(!spng_get_phys(ctx, &spng->phys)) spng->have.phys = 1;
    if(!spng_get_splt(ctx, NULL, &spng->n_splt)) spng->have.splt = 1;
    if(!spng_get_time(ctx, &spng->time)) spng->have.time = 1;
    if(!spng_get_offs(ctx, &spng->offs)) spng->have.offs = 1;
    if(!spng_get_exif(ctx, &spng->exif)) spng->have.exif = 1;
    if(!spng_get_unknown_chunks(ctx, NULL, &spng->n_unknown_chunks)) spng->have.unknown = 1;

    if(spng->have.text)
    {
        spng->text = malloc(spng->n_text * sizeof(struct spng_text));

        if(!spng->text) return 2;

        spng_get_text(ctx, spng->text, &spng->n_text);
    }

    if(spng->have.splt)
    {
        spng->splt = malloc(spng->n_splt * sizeof(struct spng_splt));

        if(!spng->splt)
        {
            free_chunks(spng);
            return 2;
        }

        spng_get_splt(ctx, spng->splt, &spng->n_splt);
    }

    if(spng->have.unknown)
    {
        spng->chunks = malloc(spng->n_unknown_chunks * sizeof(struct spng_unknown_chunk));

        if(!spng->chunks)
        {
            free_chunks(spng);
            return 2;
        }

        spng_get_unknown_chunks(ctx, spng->chunks, &spng->n_unknown_chunks);
    }

    return ret;
}

static int set_chunks(spng_ctx *dst, spngt_chunk_data *spng)
{
    int ret = 0;

    ret = spng_set_ihdr(dst, &spng->ihdr);
    if(ret)
    {
        printf("spng_set_ihdr() error: %s\n", spng_strerror(ret));
        return ret;
    }

    if(spng->have.plte) spng_set_plte(dst, &spng->plte);
    if(spng->have.trns) spng_set_trns(dst, &spng->trns);
    if(spng->have.chrm) spng_set_chrm_int(dst, &spng->chrm_int);
    if(spng->have.gama) spng_set_gama_int(dst, spng->gamma_int);
    if(spng->have.iccp) spng_set_iccp(dst, &spng->iccp);
    if(spng->have.sbit) spng_set_sbit(dst, &spng->sbit);
    if(spng->have.srgb) spng_set_srgb(dst, spng->srgb_rendering_intent);
    if(spng->have.text) spng_set_text(dst, spng->text, spng->n_text);
    if(spng->have.bkgd) spng_set_bkgd(dst, &spng->bkgd);
    if(spng->have.hist) spng_set_hist(dst, &spng->hist);
    if(spng->have.phys) spng_set_phys(dst, &spng->phys);
    if(spng->have.splt) spng_set_splt(dst, spng->splt, spng->n_splt);
    if(spng->have.time) spng_set_time(dst, &spng->time);
    if(spng->have.offs) spng_set_offs(dst, &spng->offs);
    if(spng->have.exif) spng_set_exif(dst, &spng->exif);
    if(spng->have.unknown) spng_set_unknown_chunks(dst, spng->chunks, spng->n_unknown_chunks);

    return ret;
}

static int compare_chunks(spng_ctx *ctx, png_infop info_ptr, png_structp png_ptr, int after_idat)
{
    uint32_t i;
    enum spng_errno ret = 0;
    spngt_chunk_data spng = {0};
    spngt_chunk_data png = {0};

    ret = get_chunks(ctx, &spng);
    if(ret) goto cleanup;

    png.have.ihdr = 1;

    if(png_get_valid(png_ptr, info_ptr, PNG_INFO_PLTE)) png.have.plte = 1;
    if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png.have.trns = 1;
    if(png_get_valid(png_ptr, info_ptr, PNG_INFO_cHRM)) png.have.chrm = 1;
    if(png_get_valid(png_ptr, info_ptr, PNG_INFO_gAMA)) png.have.gama = 1;
    if(png_get_valid(png_ptr, info_ptr, PNG_INFO_iCCP)) png.have.iccp = 1;
    if(png_get_valid(png_ptr, info_ptr, PNG_INFO_sBIT)) png.have.sbit = 1;
    if(png_get_valid(png_ptr, info_ptr, PNG_INFO_sRGB)) png.have.srgb = 1;
    if(png_get_valid(png_ptr, info_ptr, PNG_INFO_bKGD)) png.have.bkgd = 1;
    if(png_get_valid(png_ptr, info_ptr, PNG_INFO_hIST)) png.have.hist = 1;
    if(png_get_valid(png_ptr, info_ptr, PNG_INFO_pHYs)) png.have.phys = 1;
    if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tIME)) png.have.time = 1;
    if(png_get_valid(png_ptr, info_ptr, PNG_INFO_oFFs)) png.have.offs = 1;
    if(png_get_valid(png_ptr, info_ptr, PNG_INFO_eXIf)) png.have.exif = 1;

    png.have.unknown = spng.have.unknown;

    int png_num_palette;
    png_color *png_palette;
    if(png_get_PLTE(png_ptr, info_ptr, &png_palette, &png_num_palette) == PNG_INFO_PLTE) png.n_plte_entries = png_num_palette;

    png_text *png_text;
    png.n_text = png_get_text(png_ptr, info_ptr, &png_text, NULL);
    if(png.n_text) png.have.text = 1;

    png_sPLT_t *png_splt;
    png.n_splt = png_get_sPLT(png_ptr, info_ptr, &png_splt);
    if(png.n_splt) png.have.splt = 1;

    png_unknown_chunk *png_chunks;
    png.n_unknown_chunks = png_get_unknown_chunks(png_ptr, info_ptr, &png_chunks);
    if(png.n_unknown_chunks) png.have.unknown = 1;

    const char *pos = after_idat ? " after IDAT" : "before IDAT";

    printf("[%s] spng chunks:  ", pos);
    print_chunks(spng.have);
    printf("\n");

    printf("[%s] libpng chunks:", pos);
    print_chunks(png.have);
    printf("\n");

    if(memcmp(&spng.have, &png.have, sizeof(spngt_chunk_bitfield)))
    {
        printf("[%s] ERROR: metadata mismatch!\n", pos);
        return 1;
    }

    /* NOTE: libpng changes or corrupts chunk data once it's past the IDAT stream,
             some checks are not done because of this. */
    uint32_t png_width, png_height;
    int png_bit_depth = 0, png_color_type = 0, png_interlace_method = 0, png_compression_method = 0, png_filter_method = 0;

    png_get_IHDR(png_ptr, info_ptr, &png_width, &png_height, &png_bit_depth, &png_color_type,
                 &png_interlace_method, &png_compression_method, &png_filter_method);

    if(spng.ihdr.width != png_width ||
       spng.ihdr.height != png_height ||
       spng.ihdr.bit_depth != png_bit_depth ||
       spng.ihdr.color_type != png_color_type ||
       spng.ihdr.interlace_method != png_interlace_method ||
       spng.ihdr.compression_method != png_compression_method ||
       spng.ihdr.filter_method != png_filter_method)
    {
        if(!after_idat)
        {
            printf("IHDR data not identical\n");
            ret = 1;
            goto cleanup;
        }
    }

    if(spng.plte.n_entries != png.n_plte_entries)
    {
        printf("different number of palette entries (%u, %u)\n", spng.plte.n_entries, png.n_plte_entries);
        ret = 1;
    }
    else
    {
        for(i=0; i < spng.plte.n_entries; i++)
        {
            if(spng.plte.entries[i].red != png_palette[i].red ||
               spng.plte.entries[i].green != png_palette[i].green ||
               spng.plte.entries[i].blue != png_palette[i].blue)
            {
                printf("palette entry %d not identical\n", i);
                ret = 1;
            }
        }
    }

    if(spng.have.trns)
    {
        png_byte *png_trans_alpha;
        int png_num_trans;
        png_color_16 *png_trans_color;

        png_get_tRNS(png_ptr, info_ptr, &png_trans_alpha, &png_num_trans, &png_trans_color);

        if(spng.ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE)
        {
            if(spng.trns.gray != png_trans_color->gray)
            {
                printf("tRNS gray sample is not identical\n");
                ret = 1;
            }
        }
        else if(spng.ihdr.color_type == SPNG_COLOR_TYPE_TRUECOLOR)
        {
            if(spng.trns.red != png_trans_color->red ||
               spng.trns.green != png_trans_color->green ||
               spng.trns.blue != png_trans_color->blue)
            {
                printf("tRNS truecolor samples not identical\n");
                ret = 1;
            }
        }
        else if(spng.ihdr.color_type == SPNG_COLOR_TYPE_INDEXED)
        {
            if(spng.trns.n_type3_entries == png_num_trans)
            {
                for(i=0; i < spng.trns.n_type3_entries; i++)
                {
                    if(spng.trns.type3_alpha[i] != png_trans_alpha[i])
                    {
                        printf("tRNS alpha entry %d not identical\n", i);
                        ret = 1;
                    }
                }
            }
            else
            {
                if(!after_idat)
                {
                    printf("different number of tRNS alpha entries\n");
                    ret = 1;
                }
            }
        }
    }

    if(spng.have.chrm)
    {
        png_fixed_point png_fwhite_x, png_fwhite_y, png_fred_x, png_fred_y, png_fgreen_x, png_fgreen_y,
                        png_fblue_x, png_fblue_y;

        png_get_cHRM_fixed(png_ptr, info_ptr, &png_fwhite_x, &png_fwhite_y, &png_fred_x, &png_fred_y,
                           &png_fgreen_x, &png_fgreen_y, &png_fblue_x, &png_fblue_y);

        if(spng.chrm_int.white_point_x != png_fwhite_x ||
           spng.chrm_int.white_point_y != png_fwhite_y ||
           spng.chrm_int.red_x != png_fred_x ||
           spng.chrm_int.red_y != png_fred_y ||
           spng.chrm_int.green_x != png_fgreen_x ||
           spng.chrm_int.green_y != png_fgreen_y ||
           spng.chrm_int.blue_x != png_fblue_x ||
           spng.chrm_int.blue_y != png_fblue_y)
        {
            printf("cHRM fixed point values are not identical\n");
            ret = 1;
        }
    }

    if(spng.have.gama)
    {
        png_fixed_point png_fgamna;

        png_get_gAMA_fixed(png_ptr, info_ptr, &png_fgamna);

        if(spng.gamma_int != png_fgamna)
        {
            printf("gamma values not identical\n");
            ret = 1;
        }
    }

    if(spng.have.iccp)
    {
        png_charp png_iccp_name;
        int png_iccp_compression_type;
        png_bytep png_iccp_profile;
        png_uint_32 png_iccp_proflen;

        png_get_iCCP(png_ptr, info_ptr, &png_iccp_name, &png_iccp_compression_type, &png_iccp_profile, &png_iccp_proflen);

        if(spng.iccp.profile_len == png_iccp_proflen)
        {
            if(memcmp(spng.iccp.profile, png_iccp_profile, spng.iccp.profile_len))
            {
                printf("iccp profile data not identical\n");
                ret = 1;
            }
        }
        else
        {
            printf("iccp profile lengths are different\n");
            ret = 1;
        }
    }

    if(spng.have.sbit)
    {
        png_color_8p png_sig_bit;

        png_get_sBIT(png_ptr, info_ptr, &png_sig_bit);

        if(spng.ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE && spng.sbit.grayscale_bits != png_sig_bit->gray)
        {
            printf("grayscale significant bits not identical\n");
            ret = 1;
        }
        else if(spng.ihdr.color_type == SPNG_COLOR_TYPE_TRUECOLOR || spng.ihdr.color_type == SPNG_COLOR_TYPE_INDEXED)
        {
            if(spng.sbit.red_bits != png_sig_bit->red ||
               spng.sbit.green_bits != png_sig_bit->green ||
               spng.sbit.blue_bits != png_sig_bit->blue)
            {
                printf("rgb significant bits not identical\n");
                ret = 1;
            }
        }
        else if(spng.ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE_ALPHA)
        {
            if(spng.sbit.grayscale_bits != png_sig_bit->gray || spng.sbit.alpha_bits != png_sig_bit->alpha)
            {
                printf("grayscale alpha significant bits not identical\n");
                ret = 1;
            }
        }
        else if(spng.ihdr.color_type == 6)
        {
            if(spng.sbit.red_bits != png_sig_bit->red ||
               spng.sbit.green_bits != png_sig_bit->green ||
               spng.sbit.blue_bits != png_sig_bit->blue ||
               spng.sbit.alpha_bits != png_sig_bit->alpha)
            {
                printf("rgba significant bits not identical\n");
                ret = 1;
            }
        }
    }

    if(spng.have.srgb)
    {
        int png_rgb_intent;
        png_get_sRGB(png_ptr, info_ptr, &png_rgb_intent);

        if(spng.srgb_rendering_intent != png_rgb_intent)
        {
            printf("sRGB rendering intent mismatch\n");
            ret = 1;
        }
    }

    if(spng.n_text != png.n_text)
    {
        printf("text chunk count mismatch: %u(spng), %d(libpng)\n", spng.n_text, png.n_text);
        ret = 1;
        goto cleanup;
    }
    else
    {
        for(i=0; i < spng.n_text; i++)
        {
            if(strcmp(spng.text[i].keyword, png_text[i].key))
            {
                printf("text[%d]: keyword mismatch!\nspng: %s\n\nlibpng: %s\n", i, spng.text[i].keyword, png_text[i].key);
                ret = 1;
            }

            if(strcmp(spng.text[i].text, png_text[i].text))
            {
                printf("text[%d]: text mismatch!\nspng: %s\n\nlibpng: %s\n", i, spng.text[i].text, png_text[i].text);
                ret = 1;
            }

            if(spng.text[i].type != SPNG_ITXT) continue;

            if(strcmp(spng.text[i].language_tag, png_text[i].lang))
            {
                printf("text[%d]: language tag mismatch!\nspng: %s\n\nlibpng: %s\n", i, spng.text[i].language_tag, png_text[i].lang);
                ret = 1;
            }

            if(strcmp(spng.text[i].translated_keyword, png_text[i].lang_key))
            {
                printf("text[%d]: translated keyword mismatch!\nspng: %s\n\nlibpng: %s\n", i, spng.text[i].translated_keyword, png_text[i].lang_key);
                ret = 1;
            }
        }
    }

    if(spng.have.bkgd)
    {
        png_color_16p png_bkgd;

        png_get_bKGD(png_ptr, info_ptr, &png_bkgd);

        if(spng.ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE || spng.ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE_ALPHA)
        {
            if(spng.bkgd.gray != png_bkgd->gray)
            {
                printf("bKGD grayscale samples are not identical\n");
                ret = 1;
            }
        }
        else if(spng.ihdr.color_type == SPNG_COLOR_TYPE_TRUECOLOR || spng.ihdr.color_type == SPNG_COLOR_TYPE_TRUECOLOR_ALPHA)
        {
            if(spng.bkgd.red != png_bkgd->red ||
               spng.bkgd.green != png_bkgd->green ||
               spng.bkgd.blue != png_bkgd->blue)
            {
                printf("bKGD rgb samples are not identical\n");
                ret = 1;
            }
        }
        else if(spng.ihdr.color_type == SPNG_COLOR_TYPE_INDEXED)
        {
            if(spng.bkgd.plte_index != png_bkgd->index)
            {
                printf("bKGD type3 indices are not identical\n");
                ret = 1;
            }
        }
    }

    if(spng.have.hist)
    {
        png_uint_16p png_hist;

        png_get_hIST(png_ptr, info_ptr, &png_hist);

        for(i=0; i < spng.plte.n_entries; i++)
        {
            if(spng.hist.frequency[i] != png_hist[i])
            {
                printf("histogram entry %d is not identical\n", i);
                ret = 1;
            }
        }
    }

    if(spng.have.phys)
    {
        uint32_t png_phys_res_x, png_phys_rex_y; int png_phys_unit_type;

        png_get_pHYs(png_ptr, info_ptr, &png_phys_res_x, &png_phys_rex_y, &png_phys_unit_type);

        if(spng.phys.ppu_x != png_phys_res_x ||
           spng.phys.ppu_y != png_phys_rex_y  ||
           spng.phys.unit_specifier != png_phys_unit_type)
        {
            printf("pHYs data not indentical\n");
            ret = 1;
        }
    }

    if(spng.n_splt != png.n_splt)
    {
        printf("different number of suggested palettes\n");
        ret = 1;
    }
    else
    {
        uint32_t j;

        for(j=0; j < spng.n_splt; j++)
        {
            if(strcmp(spng.splt[j].name, png_splt[j].name))
            {
                printf("sPLT[%d]: name mismatch\n", j);
                ret = 1;
            }

            if(spng.splt[j].sample_depth != png_splt[j].depth)
            {
                printf("sPLT[%d]: sample depth mismatch\n", j);
                ret = 1;
            }

            if(spng.splt[j].n_entries != png_splt[j].nentries)
            {
                printf("sPLT[%d]: entry count mismatch\n", j);
                ret = 1;
                break;
            }

            struct spng_splt_entry entry;
            png_sPLT_entry png_entry;

            for(i=0; i < spng.splt[j].n_entries; i++)
            {
                entry = spng.splt[j].entries[i];
                png_entry = png_splt[j].entries[i];

                if(entry.alpha != png_entry.alpha ||
                    entry.red != png_entry.red ||
                    entry.green != png_entry.green ||
                    entry.blue != png_entry.blue ||
                    entry.frequency != png_entry.frequency)
                {
                    printf("sPLT[%d]: mismatch for entry %d\n", j, i);
                    ret = 1;
                }
            }
        }
    }

    if(spng.have.time)
    {
        png_time *png_time;

        png_get_tIME(png_ptr, info_ptr, &png_time);

        if(spng.time.year != png_time->year ||
           spng.time.month != png_time->month ||
           spng.time.day != png_time->day ||
           spng.time.hour != png_time->hour ||
           spng.time.minute != png_time->minute ||
           spng.time.second != png_time->second)
        {
            printf("tIME data not identical\n");
            ret = 1;
        }
    }

    if(spng.have.offs)
    {
        png_int_32 png_offset_x, png_offset_y;
        int png_offs_unit_type;

        png_get_oFFs(png_ptr, info_ptr, &png_offset_x, &png_offset_y, &png_offs_unit_type);

        if(spng.offs.x != png_offset_x ||
           spng.offs.y != png_offset_y ||
           spng.offs.unit_specifier != png_offs_unit_type)
        {
            printf("oFFs data not identical\n");
            ret = 1;
        }
    }

    if(spng.have.exif)
    {
        png_byte *png_exif;
        png_uint_32 png_exif_length;

        png_get_eXIf_1(png_ptr, info_ptr, &png_exif_length, &png_exif);

        if(spng.exif.length == png_exif_length)
        {
            if(memcmp(spng.exif.data, png_exif, spng.exif.length))
            {
                printf("eXIf data not identical\n");
                ret = 1;
            }
        }
        else
        {
            printf("eXIf chunk length mismatch\n");
            ret = 1;
        }
    }

    if(png.n_unknown_chunks != spng.n_unknown_chunks)
    {
        printf("unknown chunk count mismatch: %u(spng), %d(libpng)\n", spng.n_unknown_chunks, png.n_unknown_chunks);
        ret = 1;
        goto cleanup;
    }
    else
    {
        for(i=0; i < spng.n_unknown_chunks; i++)
        {
            if(spng.chunks[i].length != png_chunks[i].size)
            {
                printf("chunk[%d]: size mismatch %zu (spng) %zu (libpng)\n", i, spng.chunks[i].length, png_chunks[i].size);
                ret = 1;
            }

            if(spng.chunks[i].location != png_chunks[i].location)
            {
                printf("chunk[%d]: location mismatch\n", i);
                ret = 1;
            }
        }
    }

cleanup:
    free_chunks(&spng);

    return ret;
}

static int decode_and_compare(spngt_test_case *spng, spngt_test_case *png)
{
    int ret = 0;

    spng_ctx *ctx = NULL;
    size_t img_spng_size;
    unsigned char *img_spng =  NULL;

    int fmt = spng->fmt, flags = spng->flags, test_flags = spng->test_flags;

    png_infop info_ptr = NULL;
    png_structp png_ptr = NULL;
    size_t img_png_size;
    unsigned char *img_png = NULL;

    struct spng_ihdr ihdr;
    ctx = init_spng(spng, &ihdr);

    if(ctx == NULL)
    {
        ret = 1;
        goto cleanup;
    }

    spng_set_crc_action(ctx, SPNG_CRC_ERROR, SPNG_CRC_ERROR);

    img_spng = getimage_spng(ctx, &img_spng_size, fmt, flags);
    if(img_spng == NULL)
    {
        printf("getimage_spng() failed\n");
        ret = 1;
        goto cleanup;
    }

    png_ptr = init_libpng(png, &info_ptr);
    if(png_ptr == NULL)
    {
        ret = 1;
        goto cleanup;
    }

    if(test_flags & SPNGT_COMPARE_CHUNKS)
    {
        ret = compare_chunks(ctx, info_ptr, png_ptr, 0);
    }

    img_png = getimage_libpng(png_ptr, info_ptr, &img_png_size, fmt, flags);
    if(img_png == NULL)
    {
        printf("getimage_libpng() failed\n");
        ret = 1;
        goto cleanup;
    }

    if(img_png_size != img_spng_size)
    {
        printf("output image size mismatch\n");
        printf("spng: %lu\n png: %lu\n", (unsigned long int)img_spng_size, (unsigned long int)img_png_size);
        ret = 1;
        goto cleanup;
    }

    if(fmt == SPNGT_FMT_VIPS)
    {/* Get the right format for compare_images() */
        fmt = SPNG_FMT_PNG;
        if(ihdr.color_type == SPNG_COLOR_TYPE_INDEXED) fmt = SPNG_FMT_RGB8;
        else if(ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE && ihdr.bit_depth < 8) fmt = SPNG_FMT_G8;

        spng_get_trns_fmt(ctx, &fmt);
        printf("VIPS format: %s\n", fmt_str(fmt));
    }

    if(!memcmp(img_spng, img_png, img_spng_size)) goto identical;

    if( !(flags & SPNG_DECODE_GAMMA) )
    {
        printf("error: image buffers are not identical\n");
        ret = 1;
    }

    ret |= compare_images(&ihdr, fmt, flags, img_spng, img_png, img_spng_size);

    if(!ret && !(flags & SPNG_DECODE_GAMMA))
    {/* in case compare_images() has some edge case */
        printf("compare_images() returned 0 but images are not identical\n");
        ret = 1;
        goto cleanup;
    }

identical:

    if(test_flags & SPNGT_COMPARE_CHUNKS)
    {
        ret |= compare_chunks(ctx, info_ptr, png_ptr, 1);
    }

cleanup:

    spng_ctx_free(ctx);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    free(img_spng);
    free(img_png);

    return ret;
}

static void dump_buffer(void *buf, size_t size, const char *opt_name)
{
    const char *filename = opt_name ? opt_name : "dump.png";
    FILE *f = fopen(filename, "wb");

    if(f == NULL) goto err;

    size_t written = fwrite(buf, size, 1, f);

    if(written != 1) goto err;

    printf("file dumped to %s\n", filename);

    goto cleanup;

err:
    printf("failed to dump file: %s\n", strerror(errno));

cleanup:
    if(f != NULL) fclose(f);
}

static int spngt_run_test(const char *filename, struct spngt_test_case *test_case)
{
    enum spng_errno ret;
    size_t file_length = 0;
    struct spngt_test_case spng = *test_case;
    struct spngt_test_case libpng = *test_case;

    if(test_case->source.type == SPNGT_SRC_FILE)
    {
        spng.source.file = fopen(filename, "rb");
        libpng.source.file = fopen(filename, "rb");

        if(!spng.source.file || !libpng.source.file)
        {
            ret = 1;
            goto cleanup;
        }

        fseek(spng.source.file, 0, SEEK_END);
        file_length = ftell(spng.source.file);
        rewind(spng.source.file);
    }

    ret = decode_and_compare(&spng, &libpng);
    if(ret) goto cleanup;

    if(test_case->test_flags & SPNGT_ENCODE_ROUNDTRIP)
    {
        printf("           compare (reencoded, original)...\n");

        if(test_case->source.type == SPNGT_SRC_FILE)
        {
            rewind(spng.source.file);
            rewind(libpng.source.file);
        }

        spng_ctx *src = init_spng(&spng, NULL);
        spng_ctx *dst = NULL;
        spngt_chunk_data data = {0};

        size_t img_size;
        void *img_spng = getimage_spng(src, &img_size, test_case->fmt, test_case->flags);

        size_t encoded_len;
        char *encoded_pngbuf = NULL;

        if(img_spng == NULL)
        {
            ret = 1;
            goto encode_cleanup;
        }

        ret = get_chunks(src, &data);
        if(ret) goto encode_cleanup;

        dst = spng_ctx_new(SPNG_CTX_ENCODER);

        spng_set_option(dst, SPNG_ENCODE_TO_BUFFER, 1);

        ret = set_chunks(dst, &data);
        if(ret) goto encode_cleanup;

        ret = spng_encode_image(dst, img_spng, img_size, test_case->fmt, SPNG_ENCODE_FINALIZE);
        if(ret)
        {
            printf("encode error: %s\n", spng_strerror(ret));
            goto encode_cleanup;
        }

        encoded_pngbuf = spng_get_png_buffer(dst, &encoded_len, &ret);

        if(ret)
        {
            printf("failed to get encoded PNG: %s\n", spng_strerror(ret));
            goto encode_cleanup;
        }

        /* Unfortunately there's a handful of testsuite image that don't
           compress well with the default filter heuristic */
        /* Fail the test on a 4% size increase */
        /*int pct = 25;
        if( (encoded_len - encoded_len / pct) > file_length)
        {
            printf("Reencoded PNG exceeds maximum %d%% size increase: %zu (original: %zu)\n", 100 / pct, encoded_len, file_length);
            ret = 1;
            goto encode_cleanup;
        }*/
        (void)file_length;

        spng.source.type = SPNGT_SRC_BUFFER;
        spng.source.buffer = encoded_pngbuf;
        spng.source.png_size = encoded_len;
        spng.test_flags |= SPNGT_COMPARE_CHUNKS;

        ret = decode_and_compare(&spng, &libpng);
        if(ret)
        {
            printf("compare error (%d))\n", ret);
            dump_buffer(encoded_pngbuf, encoded_len, NULL);
            goto encode_cleanup;
        }

        libpng.source.type = SPNGT_SRC_BUFFER;
        libpng.source.buffer = encoded_pngbuf;
        libpng.source.png_size = encoded_len;
        libpng.test_flags |= SPNGT_COMPARE_CHUNKS;

        printf("           compare (reencoded, reencoded)...\n");

        ret = decode_and_compare(&spng, &libpng);

        if(ret)
        {
            printf("compare error (%d))\n", ret);
            dump_buffer(encoded_pngbuf, encoded_len, NULL);
        }

encode_cleanup:
        free(img_spng);
        free(encoded_pngbuf);
        spng_ctx_free(src);
        spng_ctx_free(dst);
        free_chunks(&data);
    }

    if(test_case->test_flags & SPNGT_EXTENDED_TESTS)
    {
        rewind(spng.source.file);
        ret = extended_tests(spng.source.file, test_case->fmt);
        if(ret) printf("extended tests failed\n");
    }

cleanup:

    if(spng.source.file) fclose(spng.source.file);
    if(libpng.source.file) fclose(libpng.source.file);

    return ret;
}

static int get_image_info(FILE *f, struct spng_ihdr *ihdr)
{
    spng_ctx *ctx = spng_ctx_new(0);

    spng_set_crc_action(ctx, SPNG_CRC_USE, SPNG_CRC_USE);

    spng_set_png_file(ctx, f);

    int ret = spng_get_ihdr(ctx, ihdr);

    spng_ctx_free(ctx);

    return ret;
}

int main(int argc, char **argv)
{
    if(argc < 2)
    {
        printf("no input file\n");
        return 1;
    }

    char *filename = argv[1];

    if(!strcmp(filename, "info"))
    {
        unsigned int png_ver = png_access_version_number();

        printf("spng header version: %u.%u.%u, library version: %s\n",
               SPNG_VERSION_MAJOR, SPNG_VERSION_MINOR, SPNG_VERSION_PATCH,
               spng_version_string());
        printf("png header version: %u.%u.%u, library version: %u.%u.%u\n",
               PNG_LIBPNG_VER_MAJOR, PNG_LIBPNG_VER_MINOR, PNG_LIBPNG_VER_RELEASE,
               png_ver / 10000, png_ver / 100 % 100, png_ver % 100);


        return 0;
    }

    FILE *file = fopen(filename, "rb");

    if(file == NULL)
    {
        printf("error opening input file %s\n", filename);
        return 1;
    }

    struct spng_ihdr ihdr = {0};

    if(!get_image_info(file, &ihdr))
    {
        char *clr_type_str;
        if(ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE)
            clr_type_str = "GRAY";
        else if(ihdr.color_type == SPNG_COLOR_TYPE_TRUECOLOR)
            clr_type_str = "RGB";
        else if(ihdr.color_type == SPNG_COLOR_TYPE_INDEXED)
            clr_type_str = "INDEXED";
        else if(ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE_ALPHA)
            clr_type_str = "GRAY-ALPHA";
        else
            clr_type_str = "RGBA";

        printf("%s %" PRIu8 "-bit, %" PRIu32 "x%" PRIu32 " %s\n",
               clr_type_str, ihdr.bit_depth, ihdr.width, ihdr.height,
               ihdr.interlace_method ? "interlaced" : "non-interlaced");
    }
    else printf("failed to get image info\n");

    fclose(file);

/*  With libpng it's not possible to request 8/16-bit images regardless of
    PNG format without calling functions that alias to png_set_expand(_16),
    which acts as if png_set_tRNS_to_alpha() was called, as a result
    there are no tests where transparency is not applied
*/
    int gamma_bug = 0;
    int skip_encode = 0;
    int fmt_limit = SPNGT_SKIP;
    int fmt_limit_2 = SPNGT_SKIP;

    /* https://github.com/randy408/libspng/issues/17 */
    if(ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE) gamma_bug = SPNGT_SKIP;

    /* Some output formats are limited to certain PNG formats */
    if(ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE && ihdr.bit_depth <= 8) fmt_limit = 0;
    if(ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE && ihdr.bit_depth == 16) fmt_limit_2 = 0;

    add_test_case(SPNG_FMT_PNG, 0, SPNGT_COMPARE_CHUNKS);
    add_test_case(SPNG_FMT_PNG, 0, SPNGT_ENCODE_ROUNDTRIP | skip_encode);
    add_test_case(SPNG_FMT_RAW, 0, SPNGT_ENCODE_ROUNDTRIP | skip_encode);
    add_test_case(SPNG_FMT_RAW, 0, 0);
    add_test_case(SPNG_FMT_RGBA8, SPNG_DECODE_TRNS, 0);
    add_test_case(SPNG_FMT_RGBA8, SPNG_DECODE_TRNS | SPNG_DECODE_GAMMA, gamma_bug);
    add_test_case(SPNG_FMT_RGBA16, SPNG_DECODE_TRNS, 0);
    add_test_case(SPNG_FMT_RGBA16, SPNG_DECODE_TRNS | SPNG_DECODE_GAMMA, 0);
    add_test_case(SPNG_FMT_RGB8, 0, 0);
    add_test_case(SPNG_FMT_RGB8, SPNG_DECODE_GAMMA, gamma_bug);

    add_test_case(SPNG_FMT_G8, 0, fmt_limit);
    add_test_case(SPNG_FMT_GA8, 0, fmt_limit);
    add_test_case(SPNG_FMT_GA8, SPNG_DECODE_TRNS, fmt_limit);

    add_test_case(SPNG_FMT_GA16, 0, fmt_limit_2);
    add_test_case(SPNG_FMT_GA16, SPNG_DECODE_TRNS, fmt_limit_2);

    /* This tests the input->output format logic used in libvips,
       it emulates the behavior of their old PNG loader which uses libpng. */
    add_test_case(SPNGT_FMT_VIPS, SPNG_DECODE_TRNS, 0);

    add_test_case(SPNG_FMT_PNG, 0, SPNGT_EXTENDED_TESTS);

    printf("%d test cases", n_test_cases);

    if(n_test_cases != actual_count) printf(" (skipping %d)", n_test_cases - actual_count);

    printf("\n");

    int i, ret = 0;
    for(i=0; i < n_test_cases; i++)
    {
        print_test_args(&test_cases[i]);

        if(test_cases[i].test_flags & SPNGT_SKIP) continue;

        int e = spngt_run_test(filename, &test_cases[i]);
        if(!ret) ret = e;
    }

    return ret;
}

static int stream_write_checked(spng_ctx *ctx, void *user, void *data, size_t len)
{
    (void)ctx;
    struct buf_state *state = user;

    if(len > state->bytes_left) return SPNG_IO_EOF;

    if(memcmp(data, state->data, len))
    {
        printf("stream write does not match buffer data\n");
        return SPNG_IO_ERROR;
    }

    state->data += len;
    state->bytes_left -= len;

    return 0;
}

/* Tests that don't fit anywhere else */
static int extended_tests(FILE *file, int fmt)
{
    uint32_t i;
    enum spng_errno ret = 0;
    unsigned char *image = NULL;
    unsigned char *encoded = NULL;
    spng_ctx *enc = NULL;
    spng_ctx *dec = spng_ctx_new(0);

    struct spng_ihdr ihdr = {0};
    struct spng_plte plte = {0};
    static unsigned char chunk_data[9000];

    /* NOTE: This value is compressed to 2 bits by zlib, it's not a 1:1 mapping */
    int compression_level = 0;
    int expected_compression_level = 0;

    spng_set_png_file(dec, file);

    spng_get_ihdr(dec, &ihdr);

    spng_get_plte(dec, &plte);

    size_t image_size;

    image = getimage_spng(dec, &image_size, fmt, 0);

    enc = spng_ctx_new(SPNG_CTX_ENCODER);

    spng_set_option(enc, SPNG_ENCODE_TO_BUFFER, 1);
    spng_set_option(enc, SPNG_IMG_COMPRESSION_LEVEL, compression_level);

    spng_set_ihdr(enc, &ihdr);

    if(plte.n_entries) spng_set_plte(enc, &plte);

    struct spng_unknown_chunk chunk =
    {
        .location = SPNG_AFTER_IHDR,
        .type = "cHNK",
        .length = sizeof(chunk_data),
        .data = chunk_data
    };

    uint8_t x = 0;
    for(i=0; i < chunk.length; i++)
    {
        chunk_data[i] = x;
        x++;
    }

    spng_set_unknown_chunks(enc, &chunk, 1);

    ret = spng_encode_image(enc, image, image_size, fmt, SPNG_ENCODE_FINALIZE);

    if(ret)
    {
        printf("encoding failed (%d): %s\n", ret, spng_strerror(ret));
        goto cleanup;
    }

    size_t bytes_encoded;
    encoded = spng_get_png_buffer(enc, &bytes_encoded, &ret);

    if(!encoded)
    {
        printf("getting buffer failed (%d): %s\n", ret, spng_strerror(ret));
        goto cleanup;
    }

    spng_ctx_free(enc);
    enc = NULL;

    /* Verify the image's zlib FLEVEL */
    spng_ctx_free(dec);
    dec = spng_ctx_new(0);

    spng_set_png_buffer(dec, encoded, bytes_encoded);

    spng_decode_image(dec, NULL, 0, SPNG_FMT_PNG, SPNG_DECODE_PROGRESSIVE);

    ret = spng_get_option(dec, SPNG_IMG_COMPRESSION_LEVEL, &compression_level);

    if(ret || (compression_level != expected_compression_level) )
    {
        if(ret) printf("error getting image compression level: %s\n", spng_strerror(ret));
        else
        {
            printf("unexpected compression level (expected %d, got %d)\n",
                    expected_compression_level,
                    compression_level);
            ret = 1;
        }

        goto cleanup;
    }

    /* Reencode the same image but to a stream this time */
    enc = spng_ctx_new(SPNG_CTX_ENCODER);

    struct buf_state state = { .data = encoded, .bytes_left = bytes_encoded };

    spng_set_png_stream(enc, stream_write_checked, &state);

    spng_set_option(enc, SPNG_IMG_COMPRESSION_LEVEL, compression_level);

    spng_set_ihdr(enc, &ihdr);

    if(plte.n_entries) spng_set_plte(enc, &plte);

    spng_set_unknown_chunks(enc, &chunk, 1);

    ret = spng_encode_image(enc, 0, 0, fmt, SPNG_ENCODE_PROGRESSIVE | SPNG_ENCODE_FINALIZE);

    if(ret)
    {
        printf("progressive init failed: %s\n", spng_strerror(ret));
        goto cleanup;
    }

    struct spng_row_info row_info = {0};
    size_t image_width = image_size / ihdr.height;

    do
    {
        ret = spng_get_row_info(enc, &row_info);

        if(ret) break;

        void *ptr = image + image_width * row_info.row_num;

        ret = spng_encode_row(enc, ptr, image_width);
    }while(!ret);

    if(ret == SPNG_EOI) ret = 0;

    if(ret)
    {
        printf("reencode to stream failed (%d): %s\n", ret, spng_strerror(ret));
        printf("stream offset: %zu\n", bytes_encoded - state.bytes_left);
    }

    if(spng_get_png_buffer(enc, &bytes_encoded, &ret))
    {
        printf("this should not happen\n");
        ret = 1;
        goto cleanup;
    }

    if(!ret)
    {
        printf("spng_get_png_buffer(): invalid return value\n");
        ret = 1;
        goto cleanup;
    }
    else ret = 0; /* clear the (expected) error */

    if(state.bytes_left)
    {
        printf("incomplete stream (%zu bytes shorter)\n", state.bytes_left);
        ret = 1;
    }

cleanup:
    free(image);
    free(encoded);

    spng_ctx_free(dec);
    spng_ctx_free(enc);

    return ret;
}
