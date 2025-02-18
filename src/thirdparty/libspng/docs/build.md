# Build

## Platform requirements

* Requires [zlib](http://zlib.net) or a zlib-compatible library
* Integers must be two's complement.
* Fixed width integer types up to `(u)int32_t`
* `CHAR_BIT` must equal 8.
* `size_t` must be unsigned.
* `size_t` and `int` must be at least 32-bit, 16-bit platforms are not
supported.
* Floating point support and math functions

## CMake

```bash
mkdir cbuild
cd cbuild
cmake .. # Don't forget to set optimization level!
make
make install
```

## Meson

```bash
meson build --buildtype=release # Default is debug
cd build
ninja
ninja install
```

## Embedding the source code

The source files `spng.c`/`spng.h` can be embedded in a project without
any configuration, SSE2 intrinsics are enabled by default on x86.

## Build options

| Meson       | CMake      | Compiler option             | Default | Description                                        |
|-------------|------------|-----------------------------|---------|----------------------------------------------------|
| (auto)      | (auto)     | `SPNG_STATIC`               |         | Controls symbol visibility on Windows              |
| enable_opt  | ENABLE_OPT | `SPNG_DISABLE_OPT`          | ON      | Compile with optimizations                         |
|             |            | `SPNG_SSE=<1-4>`            | 1       | SSE version target for x86 (ignored on non-x86)    |
|             |            | `SPNG_ARM`                  | (auto)  | Enable ARM NEON optimizations (ARM64 only)         |
| static_zlib |            |                             | OFF     | Link zlib statically                               |
| use_miniz   |            | `SPNG_USE_MINIZ`            | OFF     | Compile using miniz, disables some features        |
| (auto)      |            | `SPNG_ENABLE_TARGET_CLONES` |         | Use target_clones() to optimize (GCC + glibc only) |
| dev_build   |            |                             | OFF     | Enable the testsuite, requires libpng              |
| benchmarks  |            |                             | OFF     | Enable benchmarks, requires Git LFS                |
| oss_fuzz    |            |                             | OFF     | Enable regression tests with OSS-Fuzz corpora      |

Valid values for `SPNG_SSE`:

* 1 - SSE2
* 2 - same as above
* 3 - SSSE3
* 4 - SSE4.1

Currently only SSE2 optimizations are tested.

The source code alone can be built without any compiler flags,
compiler-specific macros are used to omit the need for options
such as `-msse2`, `-mssse3`.

## miniz

[miniz](https://github.com/richgel999/miniz) is a single source file replacement for zlib,
linking against miniz allows libspng to be embedded into a project with just
four files: `spng.c`, `miniz.c` and their headers.

For building with miniz add the `SPNG_USE_MINIZ` compiler option,
this handles some minor differences in the API.
Performance is mostly identical, slightly better in some cases
compared to stock zlib.

## Profile-guided optimization

[Profile-guided optimization (PGO)](https://clang.llvm.org/docs/UsersManual.html#profile-guided-optimization)
improves performance by up to 10%.

```bash
# Run in root directory
git clone https://github.com/libspng/benchmark_images.git
cd build
meson configure -Dbuildtype=release --default-library both -Db_pgo=generate
ninja
./example ../benchmark_images/medium_rgb8.png
./example ../benchmark_images/medium_rgba8.png
./example ../benchmark_images/large_palette.png
meson configure -Db_pgo=use
ninja
ninja install
```

## Documentation

Documentation is built with [mkdocs](https://www.mkdocs.org/):

```bash
# Run in root directory
mkdocs build
```