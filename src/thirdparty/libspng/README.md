[![Gitter](https://badges.gitter.im/libspng/community.svg)](https://gitter.im/libspng/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)
[![Financial Contributors on Open Collective](https://opencollective.com/libspng/all/badge.svg?label=financial+contributors)](https://opencollective.com/libspng)
[![GitLab CI](https://gitlab.com/randy408/libspng-ci/badges/master/pipeline.svg)](https://gitlab.com/randy408/libspng-ci/pipelines/latest)
[![Fuzzing Status](https://oss-fuzz-build-logs.storage.googleapis.com/badges/libspng.svg)](https://oss-fuzz-build-logs.storage.googleapis.com/index.html#libspng)
[![Coverity](https://scan.coverity.com/projects/15336/badge.svg)](https://scan.coverity.com/projects/randy408-libspng)
[![coverage](https://gitlab.com/randy408/libspng-ci/badges/master/coverage.svg)](https://gitlab.com/randy408/libspng-ci/-/graphs/master/charts)
[![tag](https://img.shields.io/github/tag-date/randy408/libspng.svg)](https://libspng.org/download)

# libspng

lib**spng** (**s**imple **png**) is a C library for reading and writing Portable Network Graphics (PNG)
format files with a focus on security and ease of use.

libspng is an alternative to libpng, the projects are separate and the APIs are
not compatible.

## Motivation

The goal is to provide a fast PNG library with a simpler API than [libpng](https://github.com/glennrp/libpng/blob/libpng16/png.h),
it outperforms the reference implementation in common use cases.

## Performance

![](https://libspng.org/perfx86.png)

## Features

| Feature                              |  spng   | libpng             | stb_image | lodepng |
|--------------------------------------|---------|--------------------|-----------|---------|
| Decode from stream                   | ✅      |  ✅               | ✅       | ❌      |
| Gamma correction                     | ✅      |  ✅               | ❌       | ❌      |
| No known security bugs<sup>[1]</sup> | ✅      |  ✅               | ❌       | ✅      |
| Progressive image read               | ✅      |  ✅               | ❌       | ❌      |
| Parses all standard chunks           | ✅      |  ✅               | ❌       | ❌      |
| Doesn't require zlib<sup>[2]</sup>   | ✅      |  ❌               | ✅       | ✅      |
| Encoding                             | ✅      |  ✅               | ✅       | ✅      |
| Animated PNG                         | Planned  |  ✅<sup>[3]</sup> | ❌       | ❌      |

<sup>[1]</sup> The project is fuzz tested on [OSS-Fuzz](https://github.com/google/oss-fuzz) and vulnerabilities are fixed before they become public.

<sup>[2]</sup> Building with miniz is [supported](docs/build.md#miniz).

<sup>[3]</sup> With a 3rd party patch

## Getting spng

Download the [latest release](https://libspng.org/download) and include `spng.c/spng.h` in your project,
you can also build with CMake or Meson, refer to the [documentation](docs/build.md) for details.

## Usage

```c
#include <spng.h>

/* Create a decoder context */
spng_ctx *ctx = spng_ctx_new(0);

/* Set an input buffer */
spng_set_png_buffer(ctx, buf, buf_size);

/* Determine output image size */
spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &out_size);

/* Decode to 8-bit RGBA */
spng_decode_image(ctx, out, out_size, SPNG_FMT_RGBA8, 0);

/* Free context memory */
spng_ctx_free(ctx);


/* Creating an encoder context requires a flag */
spng_ctx *enc = spng_ctx_new(SPNG_CTX_ENCODER);

/* Encode to internal buffer managed by the library */
spng_set_option(enc, SPNG_ENCODE_TO_BUFFER, 1);

/* Specify image dimensions, PNG format */
struct spng_ihdr ihdr =
{
    .height = height,
    .width = width,
    .bit_depth = 8,
    .color_type = SPNG_COLOR_TYPE_TRUECOLOR_ALPHA
};

/* Image will be encoded according to ihdr.color_type, .bit_depth */
spng_set_ihdr(enc, &ihdr);

/* SPNG_FMT_PNG is a special value that matches the format in ihdr,
   SPNG_ENCODE_FINALIZE will finalize the PNG with the end-of-file marker */
spng_encode_image(enc, image, image_size, SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE);

/* PNG is written to an internal buffer by default */
void *png = spng_get_png_buffer(enc, &png_size, &error);

/* User owns the buffer after a successful call */
free(png);

/* Free context memory */
spng_ctx_free(enc);
```

See [example.c](examples/example.c).

## License

Code is licensed under the BSD 2-clause "Simplified" License.

The project contains optimizations and test images from libpng, these are licensed under the
[PNG Reference Library License version 2](http://www.libpng.org/pub/png/src/libpng-LICENSE.txt).

## Security & Testing

Code is written according to the rules of the
[CERT C Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c/SEI+CERT+C+Coding+Standard).
All integer arithmetic is checked for overflow and all error conditions are handled gracefully.

The library is continuously fuzzed by [OSS-Fuzz](https://github.com/google/oss-fuzz),
code is also scanned with Clang Static Analyzer, Coverity Scan and PVS-Studio.

The test suite consists of over 1000 test cases,
175 [test images](http://www.schaik.com/pngsuite/) are decoded with all possible
output format and flag combinations and compared against libpng for [correctness](tests/README.md#Correctness).

## Versioning

Releases follow the [semantic versioning](https://semver.org/) scheme with additional guarantees:

* Releases from 0.4.0 to 0.8.x are stable
* If 1.0.0 will introduce breaking changes then 0.8.x will be maintained as a separate stable branch

Currently 1.0.0 is planned to be [compatible](https://github.com/randy408/libspng/issues/3).

## Documentation

Online documentation is available at [libspng.org/docs](https://libspng.org/docs).

## Known Issues

* Decoding to `SPNG_FMT_G8`, `SPNG_FMT_GA8` and `SPNG_FMT_GA16` output formats is not supported
 for all PNG color type and bit depth combinations, see documentation for details.
* Gamma correction is not implemented for `SPNG_FMT_PNG`, `SPNG_FMT_G8`, `SPNG_FMT_GA8`
 and `SPNG_FMT_GA16` output formats.

## Supporting spng

You can sponsor development through [OpenCollective](https://opencollective.com/libspng/), funds will be used for maintenance and further development according to the [roadmap](https://github.com/randy408/libspng/milestones).

### OpenCollective backers

#### Individuals

<a href="https://opencollective.com/libspng"><img src="https://opencollective.com/libspng/backers.svg?width=890"></a>

#### Organizations

Support this project with your organization. Your logo will show up here with a link to your website. [[Contribute](https://opencollective.com/libspng/contribute)]

<a href="https://opencollective.com/libspng/sponsor/0/website"><img src="https://opencollective.com/libspng/sponsor/0/avatar.svg"></a>
<a href="https://opencollective.com/libspng/sponsor/1/website"><img src="https://opencollective.com/libspng/sponsor/1/avatar.svg"></a>
<a href="https://opencollective.com/libspng/sponsor/2/website"><img src="https://opencollective.com/libspng/sponsor/2/avatar.svg"></a>
<a href="https://opencollective.com/libspng/sponsor/3/website"><img src="https://opencollective.com/libspng/sponsor/3/avatar.svg"></a>
<a href="https://opencollective.com/libspng/sponsor/4/website"><img src="https://opencollective.com/libspng/sponsor/4/avatar.svg"></a>
<a href="https://opencollective.com/libspng/sponsor/5/website"><img src="https://opencollective.com/libspng/sponsor/5/avatar.svg"></a>
<a href="https://opencollective.com/libspng/sponsor/6/website"><img src="https://opencollective.com/libspng/sponsor/6/avatar.svg"></a>
<a href="https://opencollective.com/libspng/sponsor/7/website"><img src="https://opencollective.com/libspng/sponsor/7/avatar.svg"></a>
<a href="https://opencollective.com/libspng/sponsor/8/website"><img src="https://opencollective.com/libspng/sponsor/8/avatar.svg"></a>
<a href="https://opencollective.com/libspng/sponsor/9/website"><img src="https://opencollective.com/libspng/sponsor/9/avatar.svg"></a>
