if get_option('build_examples') == false and get_option('dev_build') == false
    subdir_done()
endif

example_exe = executable('example', 'example.c', dependencies : spng_dep)