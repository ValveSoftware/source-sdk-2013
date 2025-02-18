cmake_minimum_required(VERSION 3.0)

project(libspng C)

set(SPNG_MAJOR 0)
set(SPNG_MINOR 7)
set(SPNG_REVISION 2)
set(SPNG_VERSION ${SPNG_MAJOR}.${SPNG_MINOR}.${SPNG_REVISION})

option(ENABLE_OPT "Enable architecture-specific optimizations" ON)
option(SPNG_SHARED "Build shared lib" ON)
option(SPNG_STATIC "Build static lib" ON)
option(BUILD_EXAMPLES "Build examples" ON)

include(GNUInstallDirs)

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

set(spng_SOURCES spng/spng.c)

if(NOT CMAKE_HOST_WIN32)
    set(spng_LIBS -lm ${ZLIB_LIBRARIES})
else()
    set(spng_LIBS ${ZLIB_LIBRARIES})
endif()

if(NOT ENABLE_OPT)
    add_definitions( -DSPNG_DISABLE_OPT=1 )
endif()

if(SPNG_SHARED)
    add_library(spng SHARED ${spng_SOURCES})
    target_link_libraries(spng ${spng_LIBS})
    install(TARGETS spng DESTINATION lib)

    if(BUILD_EXAMPLES)
        add_executable(example examples/example.c)
        target_include_directories(example PRIVATE ${PROJECT_SOURCE_DIR}/spng)
        target_link_libraries(example spng ${spng_LIBS})
    endif()
endif()

if(SPNG_STATIC)
    add_library(spng_static STATIC ${spng_SOURCES})
    target_compile_definitions(spng_static PUBLIC SPNG_STATIC)
    install(TARGETS spng_static DESTINATION lib)
endif()

install(FILES spng/spng.h DESTINATION include)


if(NOT CMAKE_HOST_WIN32 OR CYGWIN OR MINGW)
    set(prefix      ${CMAKE_INSTALL_PREFIX})
    set(exec_prefix ${CMAKE_INSTALL_PREFIX})
    set(libdir      ${CMAKE_INSTALL_FULL_LIBDIR})
    set(includedir  ${CMAKE_INSTALL_FULL_INCLUDEDIR})
    set(LIBS        "-lz -lm")

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tests/libspng.pc.in ${CMAKE_CURRENT_BINARY_DIR}/tests/libspng.pc @ONLY)

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tests/libspng.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()
