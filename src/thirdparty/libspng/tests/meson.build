if get_option('dev_build') == false
    subdir_done()
endif

add_languages('cpp', native : false)
cpp = meson.get_compiler('cpp')

fuzzer_args = []

if cc.has_function('fmemopen', prefix : '#include <stdio.h>', args : '-D_GNU_SOURCE')
    fuzzer_args = [ '-D_GNU_SOURCE', '-DSPNGT_HAVE_FMEMOPEN' ]
endif

read_fuzzer = executable('fuzz_repro',
    'fuzz_main.c',
    'spng_read_fuzzer.c',
    c_args : fuzzer_args,
    link_with : spng_lib
)

write_fuzzer_exe = executable('fuzz_write_repro',
    'fuzz_main.c',
    'spng_write_fuzzer.c',
    c_args : fuzzer_args,
    link_with : spng_lib
)

png_dep = dependency('libpng', version : '>=1.6.0', fallback : ['libpng', 'png_dep'])

test_deps = [ spng_dep, png_dep ]

test_exe = executable('testsuite', 'testsuite.c', dependencies : test_deps)

test('info', test_exe, args : 'info')

cpp_exe = executable('cpp_exe', 'test.cpp', dependencies : spng_dep)
test('cpp_test', cpp_exe)

subdir('images')
subdir('crashers')
subdir('misc')

if get_option('oss_fuzz') == false
    subdir_done()
endif

corpora = subproject('fuzzing_corpora').get_variable('corpora')

foreach case : corpora
    test('testcase', read_fuzzer, args : case)
endforeach