#!/bin/bash -eu

# This script is meant to be run by
# https://github.com/google/oss-fuzz/blob/master/projects/libspng/Dockerfile

mkdir $SRC/zlib/build $SRC/libspng/build
cd $SRC/zlib/build
cmake ..
make -j$(nproc)

$CC $CFLAGS -c -I$SRC/zlib/build -I$SRC/zlib \
    $SRC/libspng/spng/spng.c \
    -o $SRC/libspng/build/libspng.o \

cp $SRC/zlib/build/libz.a $SRC/libspng/build/libz.a
cd $SRC/libspng/build
ar x libz.a
ar rcs libspng_static.a *.o

$CXX $CXXFLAGS -std=c++11 \
    $SRC/libspng/tests/spng_read_fuzzer.c \
    -DSPNGT_HAVE_FMEMOPEN=1 \
    -o $OUT/spng_read_fuzzer \
    $LIB_FUZZING_ENGINE $SRC/libspng/build/libspng_static.a $SRC/zlib/build/libz.a

$CXX $CXXFLAGS -std=c++11 -I$SRC/zlib/build -I$SRC/zlib \
    $SRC/libspng/tests/spng_read_fuzzer.c \
    -DSPNGT_HAVE_FMEMOPEN=1 \
    -o $OUT/spng_read_fuzzer_structure_aware \
    -include $SRC/fuzzer-test-suite/libpng-1.2.56/png_mutator.h \
    -D PNG_MUTATOR_DEFINE_LIBFUZZER_CUSTOM_MUTATOR \
    $LIB_FUZZING_ENGINE $SRC/libspng/build/libspng_static.a $SRC/zlib/build/libz.a

$CXX $CXXFLAGS -std=c++11 \
    $SRC/libspng/tests/spng_write_fuzzer.c \
    -DSPNGT_HAVE_FMEMOPEN=1 \
    -o $OUT/spng_write_fuzzer \
    $LIB_FUZZING_ENGINE $SRC/libspng/build/libspng_static.a $SRC/zlib/build/libz.a


find $SRC/libspng/tests -name "*.png" | \
     xargs zip $OUT/seed_corpus.zip

cp $SRC/libspng/tests/spng.dict $OUT/

ln -sf $OUT/seed_corpus.zip $OUT/spng_read_fuzzer_seed_corpus.zip
ln -sf $OUT/seed_corpus.zip $OUT/spng_read_fuzzer_structure_aware_seed_corpus.zip

ln -sf $OUT/spng.dict $OUT/spng_read_fuzzer.dict
ln -sf $OUT/spng.dict $OUT/spng_read_fuzzer_structure_aware.dict
