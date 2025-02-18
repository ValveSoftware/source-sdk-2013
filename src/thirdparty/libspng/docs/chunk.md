# Semantics

* Chunk data is stored in `spng_ctx`.

* All `spng_get_*()` functions return 0 on success and non-zero error,
  `SPNG_ECHUNKAVAIL` if the PNG does not contain that chunk or was not previously
  set.
* A successful `spng_set_*()` call will replace any previously set value or list,
  it does not combine chunk data from the file or multiple `spng_set_*()` calls.

The following apply to decoder contexts:

* When calling `spng_get_*()` or `spng_set_*()` functions all
  chunks up to the first IDAT are read, validated then stored
  with the exception of `spng_get_ihdr()`, which only reads the header.
* When calling `spng_get_*()` after the image has been decoded all
  chunks up to the IEND marker are read.
* `spng_set_*()` functions replace stored chunk data for that type.
* Chunk data stored with `spng_set_*()` functions are never replaced with
  input file chunk data i.e. if you set something it will stay that way.

# API

# spng_get_ihdr()
```c
int spng_get_ihdr(spng_ctx *ctx, struct spng_ihdr *ihdr)
```

Get image header

# spng_get_plte()
```c
int spng_get_plte(spng_ctx *ctx, struct spng_plte *plte)
```

Get image palette

# spng_get_trns()
```c
int spng_get_trns(spng_ctx *ctx, struct spng_trns *trns)
```

Get image transparency

# spng_get_chrm()
```c
int spng_get_chrm(spng_ctx *ctx, struct spng_chrm *chrm)
```

Get primary chromacities and white point as floating point numbers

# spng_get_chrm_int()
```c
int spng_get_chrm_int(spng_ctx *ctx, struct spng_chrm_int *chrm_int)
```

Get primary chromacities and white point in PNG's internal representation

# spng_get_gama()
```c
int spng_get_gama(spng_ctx *ctx, double *gamma)
```

Get image gamma

# spng_get_gama_int()
```c
int spng_get_gama_int(spng_ctx *ctx, uint32_t *gamma)
```

Get image gamma in PNG's internal representation

# spng_get_iccp()
```c
int spng_get_iccp(spng_ctx *ctx, struct spng_iccp *iccp)
```

Get ICC profile

!!! note
    ICC profiles are not validated.

# spng_get_sbit()
```c
int spng_get_sbit(spng_ctx *ctx, struct spng_sbit *sbit)
```

Get significant bits

# spng_get_srgb()
```c
int spng_get_srgb(spng_ctx *ctx, uint8_t *rendering_intent)
```

Get rendering intent

# spng_get_text()
```c
int spng_get_text(spng_ctx *ctx, struct spng_text *text, uint32_t *n_text)
```

Copies text information to `text`.

`n_text` should be greater than or equal to the number of stored text chunks.

If `text` is NULL and `n_text` is non-NULL then `n_text` is set to the number
of stored text chunks.

Strings may be zero-length (single `'\0'` character) with the exception of `text.keyword`,
all strings are guaranteed to be non-NULL.

!!! note
    Due to the structure of PNG files it is recommended to call this function
    after `spng_decode_image()` to retrieve all text chunks.

!!! warning
    Text data is freed when calling `spng_ctx_free()`.

# spng_get_bkgd()
```c
int spng_get_bkgd(spng_ctx *ctx, struct spng_bkgd *bkgd)
```

Get image background color

# spng_get_hist()
```c
int spng_get_hist(spng_ctx *ctx, struct spng_hist *hist)
```

Get image histogram

# spng_get_phys()
```c
int spng_get_phys(spng_ctx *ctx, struct spng_phys *phys)
```

Get phyiscal pixel dimensions

# spng_get_splt()
```c
int spng_get_splt(spng_ctx *ctx, struct spng_splt *splt, uint32_t *n_splt)
```

Copies suggested palettes to `splt`.

`n_splt` should be greater than or equal to the number of stored sPLT chunks.

If `splt` is NULL and `n_splt` is non-NULL then `n_splt` is set to the number
of stored sPLT chunks.

!!! warning
    Suggested palettes are freed when calling `spng_ctx_free()`.

# spng_get_time()
```c
int spng_get_time(spng_ctx *ctx, struct spng_time *time)
```

Get modification time

!!! note
    Due to the structure of PNG files it is recommended to call this function
    after `spng_decode_image()`.


# spng_get_unknown_chunks()
```c
int spng_get_unknown_chunks(spng_ctx *ctx, struct spng_unknown_chunk *chunks, uint32_t *n_chunks)
```

Copies unknown chunk information to `chunks`.

`n_chunks` should be greater than or equal to the number of stored unknown chunks.

If `chunks` is NULL and `n_chunks` is non-NULL then `n_chunks` is set to the number
of stored chunks.

!!! note
    To retrieve all unknown chunks call this functions after `spng_decode_image()`.

!!! warning
    Chunk data is freed when calling `spng_ctx_free()`.


# spng_get_offs()
```c
int spng_get_offs(spng_ctx *ctx, struct spng_offs *offs)
```

Get image offset

# spng_get_exif()
```c
int spng_get_exif(spng_ctx *ctx, struct spng_exif *exif)
```

Get EXIF data

!!! note
    Due to the structure of PNG files it is recommended to call this function
    after `spng_decode_image()`.

!!! warning
    `exif.data` is freed when calling `spng_ctx_free()`.



# spng_set_ihdr()
```c
int spng_set_ihdr(spng_ctx *ctx, struct spng_ihdr *ihdr)
```

Set image header

# spng_set_plte()
```c
int spng_set_plte(spng_ctx *ctx, struct spng_plte *plte)
```

Set image palette

# spng_set_trns()
```c
int spng_set_trns(spng_ctx *ctx, struct spng_trns *trns)
```

Set transparency

# spng_set_chrm()
```c
int spng_set_chrm(spng_ctx *ctx, struct spng_chrm *chrm)
```

Set primary chromacities and white point as floating point numbers

# spng_set_chrm_int()
```c
int spng_set_chrm_int(spng_ctx *ctx, struct spng_chrm_int *chrm_int)
```

Set primary chromacities and white point in PNG's internal representation

# spng_set_gama()
```c
int spng_set_gama(spng_ctx *ctx, double gamma)
```

Set image gamma

# spng_set_gama_int()
```c
int spng_set_gama_int(spng_ctx *ctx, uint32_t gamma)
```

Set image gamma in PNG's internal representation

# spng_set_iccp()
```c
int spng_set_iccp(spng_ctx *ctx, struct spng_iccp *iccp)
```

Set ICC profile

`spng_iccp.profile_name` must only contain printable Latin-1 characters and spaces.
Leading, trailing, and consecutive spaces are not permitted.

!!! note
    ICC profiles are not validated.


# spng_set_sbit()
```c
int spng_set_sbit(spng_ctx *ctx, struct spng_sbit *sbit)
```

Set significant bits

# spng_set_srgb()
```c
int spng_set_srgb(spng_ctx *ctx, uint8_t rendering_intent)
```

Set rendering intent

# spng_set_text()
```c
int spng_set_text(spng_ctx *ctx, struct spng_text *text, uint32_t n_text)
```

Set text data

`text` should point to an `spng_text` array of `n_text` elements.

`spng_text.text` must only contain Latin-1 characters.
Newlines must be a single linefeed character (decimal 10).

`spng_text.translated_keyword` must not contain linebreaks.

`spng_text.compression_method` must be zero.


# spng_set_bkgd()
```c
int spng_set_bkgd(spng_ctx *ctx, struct spng_bkgd *bkgd)
```

Set image background color

# spng_set_hist()
```c
int spng_set_hist(spng_ctx *ctx, struct spng_hist *hist)
```

Set image histogram

# spng_set_phys()
```c
int spng_set_phys(spng_ctx *ctx, struct spng_phys *phys)
```

Set phyiscal pixel dimensions

# spng_set_splt()
```c
int spng_set_splt(spng_ctx *ctx, struct spng_splt *splt, uint32_t n_splt)
```

Set suggested palette(s).

`splt` should point to an `spng_splt` array of `n_splt` elements.

!!! note
    `splt` should be a valid reference for the lifetime of the context.

# spng_set_time()
```c
int spng_set_time(spng_ctx *ctx, struct spng_time *time)
```

Set modification time

# spng_set_unknown_chunks()
```c
int spng_set_unknown_chunks(spng_ctx *ctx, struct spng_unknown_chunk *chunks, uint32_t n_chunks)
```

Set unknown chunk data.

!!! note
    `chunks` should be a valid reference for the lifetime of the context.


# spng_set_offs()
```c
int spng_set_offs(spng_ctx *ctx, struct spng_offs *offs)
```

Set image offset

# spng_set_exif()
```c
int spng_set_exif(spng_ctx *ctx, struct spng_exif *exif)
```
Set EXIF data
