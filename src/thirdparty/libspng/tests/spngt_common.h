#ifndef SPNGT_COMMON_H
#define SPNGT_COMMON_H

#include <spng.h>

#include <inttypes.h>

#define SPNG_FMT_RGB16 8 /* XXX: Remove when support is added */

#define SPNGT_FMT_VIPS (1 << 20) /* the sequence of libpng calls in libvips */

typedef struct spngt_chunk_bitfield
{
    unsigned ihdr: 1;
    unsigned plte: 1;
    unsigned trns: 1;
    unsigned chrm: 1;
    unsigned gama: 1;
    unsigned iccp: 1;
    unsigned sbit: 1;
    unsigned srgb: 1;
    unsigned text: 1;
    unsigned ztxt: 1;
    unsigned itxt: 1;
    unsigned bkgd: 1;
    unsigned hist: 1;
    unsigned phys: 1;
    unsigned splt: 1;
    unsigned time: 1;
    unsigned offs: 1;
    unsigned exif: 1;
    unsigned unknown: 1;
}spngt_chunk_bitfield;

typedef struct spngt_chunk_data
{
    uint32_t n_text, n_splt, n_plte_entries, n_unknown_chunks;
    spngt_chunk_bitfield have;

    /* only meant to be set by spng */
    struct spng_ihdr ihdr;
    struct spng_plte plte;
    struct spng_trns trns;
    struct spng_chrm chrm;
    struct spng_chrm_int chrm_int;
    double gamma;
    uint32_t gamma_int;
    struct spng_iccp iccp;
    struct spng_sbit sbit;
    uint8_t srgb_rendering_intent;
    struct spng_text *text;
    struct spng_bkgd bkgd;
    struct spng_hist hist;
    struct spng_phys phys;
    struct spng_splt *splt;
    struct spng_time time;
    struct spng_offs offs;
    struct spng_exif exif;
    struct spng_unknown_chunk *chunks;
}spngt_chunk_data;

enum spngt_flags
{
    SPNGT_COMPARE_CHUNKS = 1,
    SPNGT_PRELOAD_FILE = 2, /* Preload PNG from file and/or decode from buffer */
    SPNGT_ENCODE_ROUNDTRIP = 4,
    SPNGT_EXTENDED_TESTS = 8,
    SPNGT_SKIP = 8192
};

enum spngt_source_type
{
    SPNGT_SRC_FILE = 0,
    SPNGT_SRC_BUFFER
};

struct spngt_source
{
    enum spngt_source_type type;

    FILE *file;
    void *buffer;

    size_t png_size;
};

typedef struct spngt_test_case
{
    struct spngt_source source;

    int fmt;
    int flags;
    int test_flags;
}spngt_test_case;


#endif /* SPNGT_COMMON_H */
