# Testing

Unit testing requires Meson and libpng, currently only developer builds expose
the test cases, this is enabled with `meson configure -Ddev_build=true`.

All unit tests are run with the `meson test` command, to enable sanitizers:

```bash
meson configure -Db_sanitize=address,undefined
```

Testing with MemorySanitizer requires all dependencies to be instrumented,
Meson can be forced to download libpng and zlib and build them from source
using the `--wrap-mode=forcefallback` option when creating a build.

## Correctness

To ensure correctness the decoder is tested with all supported output formats
and decode flags against every PNG color type and bit depth combination.

The testsuite uses Meson's unit testing framework,
images from the [PngSuite](http://web.archive.org/web/20200414214727/www.schaik.com/pngsuite/)
and libpng to verify the results.

For each PNG test cases are created with unique libspng output format and decode flag combinations,
a translation layer converts these to libpng calls and the image is decoded with both libraries.

To pass each test the output images have to be bit-identical,
for gamma-corrected images each color / grayscale sample has to be within 2%.

The testsuite also covers deinterlacing with 1, 2, 4-bit samples.

## Regression tests

The `crashers` directory contains regressions tests, some of these files
were copied from the libpng repository.

## Fuzz testing

Code is continuously fuzzed on [OSS-Fuzz](https://google.github.io/oss-fuzz/)
using [`spng_read_fuzzer.c`](spng_read_fuzzer.c) as the fuzz target,
[code coverage](https://oss-fuzz.com/coverage-report/job/libfuzzer_asan_libspng/latest) information is constantly updated.

Pull requests are also tested with [CIFuzz](https://google.github.io/oss-fuzz/getting-started/continuous-integration/),
this runs a short fuzz test and catches most bugs before they could be merged.

The `fuzz_repro` executable is used for reproducing test cases,
it uses a dummy entrypoint to replace the libFuzzer dependency.

## Fuzzing corpora

Regression tests can be run against the fuzzing corpora created by OSS-Fuzz,
this is enabled with the `oss_fuzz` option, the tests are run with the
same `meson test` command.