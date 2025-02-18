project('spng', 'c',
    version : '0.7.2',
    license : [ 'BSD-2-Clause', 'libpng-2.0' ],
    default_options : 'c_std=c99'
)

spng_args = []
static_subproject = false

if get_option('default_library') == 'static'
    spng_args = '-DSPNG_STATIC'
    static_subproject = meson.is_subproject()
endif

if get_option('enable_opt') == false
    add_project_arguments('-DSPNG_DISABLE_OPT', language : 'c')
endif

cc = meson.get_compiler('c')

# Check for GNU target_clones attribute
if cc.compiles(files('tests/target_clones.c'), args : '-Werror', name : 'have target_clones')
    add_project_arguments('-DSPNG_ENABLE_TARGET_CLONES', language : 'c')
endif

if get_option('use_miniz') == true
    add_project_arguments('-DSPNG_USE_MINIZ', language : 'c')
    zlib_dep = dependency('miniz', fallback : [ 'miniz', 'miniz_dep'])
else
    zlib_dep = dependency('zlib',
        required : false,
        fallback : ['zlib', 'zlib_dep'],
        static : get_option('static_zlib'))

    if not zlib_dep.found()
        zlib_dep = cc.find_library('z')
    endif
endif

m_dep = cc.find_library('m', required : false)

spng_deps = [ zlib_dep, m_dep ]

spng_inc = include_directories('spng')

spng_src = files('spng/spng.c')

spng_lib = library('spng',
    spng_src,
    c_args : spng_args,
    dependencies : spng_deps,
    install : not static_subproject,
    version : '0.7.2'
)

spng_dep = declare_dependency(
    link_with : spng_lib,
    compile_args : spng_args,
    include_directories : spng_inc,
    version : meson.project_version()
)

if meson.version().version_compare('>= 0.54.0')
    meson.override_dependency('spng', spng_dep)
endif

subdir('examples')
subdir('tests')

if get_option('benchmarks') == true
    subproject('spngt')
endif

if static_subproject
    subdir_done()
endif

install_headers('spng/spng.h')

pkg = import('pkgconfig')

pkg.generate(spng_lib,
    extra_cflags : spng_args,
    libraries_private : [ '-lm', '-lz' ],
    description : 'PNG decoding and encoding library'
)
