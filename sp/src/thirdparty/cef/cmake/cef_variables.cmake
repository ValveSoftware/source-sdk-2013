# Copyright (c) 2016 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

# Must be loaded via FindCEF.cmake.
if(NOT DEFINED _CEF_ROOT_EXPLICIT)
  message(FATAL_ERROR "Use find_package(CEF) to load this file.")
endif()


#
# Shared configuration.
#

# Determine the platform.
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
  set(OS_MACOSX 1)
  set(OS_POSIX 1)
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
  set(OS_LINUX 1)
  set(OS_POSIX 1)
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  set(OS_WINDOWS 1)
endif()

# Determine the project architecture.
if(NOT DEFINED PROJECT_ARCH)
  if(CMAKE_SIZEOF_VOID_P MATCHES 8)
    set(PROJECT_ARCH "x86_64")
  else()
    set(PROJECT_ARCH "x86")
  endif()

  if(OS_MACOSX)
    # PROJECT_ARCH should be specified on Mac OS X.
    message(WARNING "No PROJECT_ARCH value specified, using ${PROJECT_ARCH}")
  endif()
endif()

# Determine the build type.
if(NOT CMAKE_BUILD_TYPE AND
   (${CMAKE_GENERATOR} STREQUAL "Ninja" OR ${CMAKE_GENERATOR} STREQUAL "Unix Makefiles"))
  # CMAKE_BUILD_TYPE should be specified when using Ninja or Unix Makefiles.
  set(CMAKE_BUILD_TYPE Release)
  message(WARNING "No CMAKE_BUILD_TYPE value selected, using ${CMAKE_BUILD_TYPE}")
endif()


# Path to the include directory.
set(CEF_INCLUDE_PATH "${_CEF_ROOT}")

# Path to the libcef_dll_wrapper target.
set(CEF_LIBCEF_DLL_WRAPPER_PATH "${_CEF_ROOT}/libcef_dll")


# Shared compiler/linker flags.
list(APPEND CEF_COMPILER_DEFINES
  # Allow C++ programs to use stdint.h macros specified in the C99 standard that aren't 
  # in the C++ standard (e.g. UINT8_MAX, INT64_MIN, etc)
  __STDC_CONSTANT_MACROS __STDC_FORMAT_MACROS
  )


#
# Linux configuration.
#

if(OS_LINUX)
  # Platform-specific compiler/linker flags.
  set(CEF_LIBTYPE SHARED)
  list(APPEND CEF_COMPILER_FLAGS
    -fno-strict-aliasing            # Avoid assumptions regarding non-aliasing of objects of different types
    -fPIC                           # Generate position-independent code for shared libraries
    -fstack-protector               # Protect some vulnerable functions from stack-smashing (security feature)
    -funwind-tables                 # Support stack unwinding for backtrace()
    -fvisibility=hidden             # Give hidden visibility to declarations that are not explicitly marked as visible
    --param=ssp-buffer-size=4       # Set the minimum buffer size protected by SSP (security feature, related to stack-protector)
    -pipe                           # Use pipes rather than temporary files for communication between build stages
    -pthread                        # Use the pthread library
    -Wall                           # Enable all warnings
    -Werror                         # Treat warnings as errors
    -Wno-missing-field-initializers # Don't warn about missing field initializers
    -Wno-unused-parameter           # Don't warn about unused parameters
    )
  list(APPEND CEF_C_COMPILER_FLAGS
    -std=c99                        # Use the C99 language standard
    )
  list(APPEND CEF_CXX_COMPILER_FLAGS
    -fno-exceptions                 # Disable exceptions
    -fno-rtti                       # Disable real-time type information
    -fno-threadsafe-statics         # Don't generate thread-safe statics
    -fvisibility-inlines-hidden     # Give hidden visibility to inlined class member functions
    -std=gnu++11                    # Use the C++11 language standard including GNU extensions
    -Wsign-compare                  # Warn about mixed signed/unsigned type comparisons
    )
  list(APPEND CEF_COMPILER_FLAGS_DEBUG
    -O0                             # Disable optimizations
    -g                              # Generate debug information
    )
  list(APPEND CEF_COMPILER_FLAGS_RELEASE
    -O2                             # Optimize for maximum speed
    -fdata-sections                 # Enable linker optimizations to improve locality of reference for data sections
    -ffunction-sections             # Enable linker optimizations to improve locality of reference for function sections
    -fno-ident                      # Ignore the #ident directive
    -U_FORTIFY_SOURCE               # Undefine _FORTIFY_SOURCE in case it was previously defined
    -D_FORTIFY_SOURCE=2             # Add memory and string function protection (security feature, related to stack-protector)
    )
  list(APPEND CEF_LINKER_FLAGS
    -fPIC                           # Generate position-independent code for shared libraries
    -pthread                        # Use the pthread library
    -Wl,--disable-new-dtags         # Don't generate new-style dynamic tags in ELF
    -Wl,--fatal-warnings            # Treat warnings as errors
    -Wl,-rpath,.                    # Set rpath so that libraries can be placed next to the executable
    -Wl,-z,noexecstack              # Mark the stack as non-executable (security feature)
    -Wl,-z,now                      # Resolve symbols on program start instead of on first use (security feature)
    -Wl,-z,relro                    # Mark relocation sections as read-only (security feature)
    )
  list(APPEND CEF_LINKER_FLAGS_RELEASE
    -Wl,-O1                         # Enable linker optimizations
    -Wl,--as-needed                 # Only link libraries that export symbols used by the binary
    -Wl,--gc-sections               # Remove unused code resulting from -fdata-sections and -function-sections
    )
  list(APPEND CEF_COMPILER_DEFINES
    _FILE_OFFSET_BITS=64            # Allow the Large File Support (LFS) interface to replace the old interface
    )
  list(APPEND CEF_COMPILER_DEFINES_RELEASE
    NDEBUG                          # Not a debug build
    )

  include(CheckCCompilerFlag)
  include(CheckCXXCompilerFlag)

  CHECK_C_COMPILER_FLAG(-Wno-unused-local-typedefs COMPILER_SUPPORTS_NO_UNUSED_LOCAL_TYPEDEFS)
  if(COMPILER_SUPPORTS_NO_UNUSED_LOCAL_TYPEDEFS)
    list(APPEND CEF_C_COMPILER_FLAGS
      -Wno-unused-local-typedefs  # Don't warn about unused local typedefs
      )
  endif()

  CHECK_CXX_COMPILER_FLAG(-Wno-literal-suffix COMPILER_SUPPORTS_NO_LITERAL_SUFFIX)
  if(COMPILER_SUPPORTS_NO_LITERAL_SUFFIX)
    list(APPEND CEF_CXX_COMPILER_FLAGS
      -Wno-literal-suffix         # Don't warn about invalid suffixes on literals
      )
  endif()

  CHECK_CXX_COMPILER_FLAG(-Wno-narrowing COMPILER_SUPPORTS_NO_NARROWING)
  if(COMPILER_SUPPORTS_NO_NARROWING)
    list(APPEND CEF_CXX_COMPILER_FLAGS
      -Wno-narrowing              # Don't warn about type narrowing
      )
  endif()

  if(PROJECT_ARCH STREQUAL "x86_64")
    # 64-bit architecture.
    list(APPEND CEF_COMPILER_FLAGS
      -m64
      -march=x86-64
      )
    list(APPEND CEF_LINKER_FLAGS
      -m64
      )
  elseif(PROJECT_ARCH STREQUAL "x86")
    # 32-bit architecture.
    list(APPEND CEF_COMPILER_FLAGS
      -msse2
      -mfpmath=sse
      -mmmx
      -m32
      )
    list(APPEND CEF_LINKER_FLAGS
      -m32
      )
  endif()

  # Standard libraries.
  set(CEF_STANDARD_LIBS
    X11
    )

  # CEF directory paths.
  set(CEF_RESOURCE_DIR        "${_CEF_ROOT}/Resources")
  set(CEF_BINARY_DIR          "${_CEF_ROOT}/${CMAKE_BUILD_TYPE}")
  set(CEF_BINARY_DIR_DEBUG    "${_CEF_ROOT}/Debug")
  set(CEF_BINARY_DIR_RELEASE  "${_CEF_ROOT}/Release")

  # CEF library paths.
  set(CEF_LIB_DEBUG   "${CEF_BINARY_DIR_DEBUG}/libcef.so")
  set(CEF_LIB_RELEASE "${CEF_BINARY_DIR_RELEASE}/libcef.so")

  # List of CEF binary files.
  set(CEF_BINARY_FILES
    chrome-sandbox
    libcef.so
    natives_blob.bin
    snapshot_blob.bin
    )

  # List of CEF resource files.
  set(CEF_RESOURCE_FILES
    cef.pak
    cef_100_percent.pak
    cef_200_percent.pak
    cef_extensions.pak
    devtools_resources.pak
    icudtl.dat
    locales
    )
endif()


#
# Mac OS X configuration.
#

if(OS_MACOSX)
  # Platform-specific compiler/linker flags.
  # See also Xcode target properties in macros.cmake.
  set(CEF_LIBTYPE SHARED)
  list(APPEND CEF_COMPILER_FLAGS
    -fno-strict-aliasing            # Avoid assumptions regarding non-aliasing of objects of different types
    -fstack-protector               # Protect some vulnerable functions from stack-smashing (security feature)
    -funwind-tables                 # Support stack unwinding for backtrace()
    -fvisibility=hidden             # Give hidden visibility to declarations that are not explicitly marked as visible
    -Wall                           # Enable all warnings
    -Werror                         # Treat warnings as errors
    -Wextra                         # Enable additional warnings
    -Wendif-labels                  # Warn whenever an #else or an #endif is followed by text
    -Wnewline-eof                   # Warn about no newline at end of file
    -Wno-missing-field-initializers # Don't warn about missing field initializers
    -Wno-unused-parameter           # Don't warn about unused parameters
    )
  list(APPEND CEF_C_COMPILER_FLAGS
    -std=c99                        # Use the C99 language standard
    )
  list(APPEND CEF_CXX_COMPILER_FLAGS
    -fno-exceptions                 # Disable exceptions
    -fno-rtti                       # Disable real-time type information
    -fno-threadsafe-statics         # Don't generate thread-safe statics
    -fobjc-call-cxx-cdtors          # Call the constructor/destructor of C++ instance variables in ObjC objects
    -fvisibility-inlines-hidden     # Give hidden visibility to inlined class member functions
    -std=gnu++11                    # Use the C++11 language standard including GNU extensions
    -Wno-narrowing                  # Don't warn about type narrowing
    -Wsign-compare                  # Warn about mixed signed/unsigned type comparisons
    )
  list(APPEND CEF_COMPILER_FLAGS_DEBUG
    -O0                             # Disable optimizations
    -g                              # Generate debug information
    )
  list(APPEND CEF_COMPILER_FLAGS_RELEASE
    -O3                             # Optimize for maximum speed plus a few extras
    )
  list(APPEND CEF_LINKER_FLAGS
    -Wl,-search_paths_first         # Search for static or shared library versions in the same pass
    -Wl,-ObjC                       # Support creation of ObjC static libraries
    -Wl,-pie                        # Generate position-independent code suitable for executables only
    )
  list(APPEND CEF_LINKER_FLAGS_RELEASE
    -Wl,-dead_strip                 # Strip dead code
    )

  # Standard libraries.
  set(CEF_STANDARD_LIBS
    -lpthread
    "-framework Cocoa"
    "-framework AppKit"
    )

  # Find the newest available base SDK.
  execute_process(COMMAND xcode-select --print-path OUTPUT_VARIABLE XCODE_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
  foreach(OS_VERSION 10.10 10.9 10.8 10.7)
    set(SDK "${XCODE_PATH}/Platforms/MacOSX.platform/Developer/SDKs/MacOSX${OS_VERSION}.sdk")
    if(NOT "${CMAKE_OSX_SYSROOT}" AND EXISTS "${SDK}" AND IS_DIRECTORY "${SDK}")
      set(CMAKE_OSX_SYSROOT ${SDK})
    endif()
  endforeach()

  # Target SDK.
  set(CEF_TARGET_SDK               "10.7")
  list(APPEND CEF_COMPILER_FLAGS
    -mmacosx-version-min=${CEF_TARGET_SDK}
  )
  set(CMAKE_OSX_DEPLOYMENT_TARGET  ${CEF_TARGET_SDK})

  # Target architecture.
  if(PROJECT_ARCH STREQUAL "x86_64")
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
  else()
    set(CMAKE_OSX_ARCHITECTURES "i386")
  endif()

  # CEF directory paths.
  set(CEF_BINARY_DIR          "${_CEF_ROOT}/$<CONFIGURATION>")
  set(CEF_BINARY_DIR_DEBUG    "${_CEF_ROOT}/Debug")
  set(CEF_BINARY_DIR_RELEASE  "${_CEF_ROOT}/Release")

  # CEF library paths.
  set(CEF_LIB_DEBUG   "${CEF_BINARY_DIR_DEBUG}/Chromium Embedded Framework.framework/Chromium Embedded Framework")
  set(CEF_LIB_RELEASE "${CEF_BINARY_DIR_RELEASE}/Chromium Embedded Framework.framework/Chromium Embedded Framework")
endif()


#
# Windows configuration.
#

if(OS_WINDOWS)
  # Configure use of the sandbox.
  option(USE_SANDBOX "Enable or disable use of the sandbox." ON)
  if(USE_SANDBOX AND NOT MSVC_VERSION EQUAL 1900)
    # The cef_sandbox.lib static library is currently built with VS2015. It will
    # not link successfully with other VS versions.
    set(USE_SANDBOX OFF)
  endif()

  # Configure use of official build compiler settings.
  # When using an official build the "Debug" build is actually a Release build
  # with DCHECKs enabled. In order to link the sandbox the Debug build must
  # be configured with some Release-related compiler settings.
  option(USE_OFFICIAL_BUILD_SANDBOX "Enable or disable use of an official build sandbox." ON)
  if(NOT USE_SANDBOX)
    # Don't need official build settings when the sandbox is off.
    set(USE_OFFICIAL_BUILD_SANDBOX OFF)
  endif()

  # Consumers who run into LNK4099 warnings can pass /Z7 instead (see issue #385).
  set(CEF_DEBUG_INFO_FLAG "/Zi" CACHE STRING "Optional flag specifying specific /Z flag to use")

  # Platform-specific compiler/linker flags.
  set(CEF_LIBTYPE STATIC)
  list(APPEND CEF_COMPILER_FLAGS
    /MP           # Multiprocess compilation
    /Gy           # Enable function-level linking
    /GR-          # Disable run-time type information
    /W4           # Warning level 4
    /WX           # Treat warnings as errors
    /wd4100       # Ignore "unreferenced formal parameter" warning
    /wd4127       # Ignore "conditional expression is constant" warning
    /wd4244       # Ignore "conversion possible loss of data" warning
    /wd4481       # Ignore "nonstandard extension used: override" warning
    /wd4512       # Ignore "assignment operator could not be generated" warning
    /wd4701       # Ignore "potentially uninitialized local variable" warning
    /wd4702       # Ignore "unreachable code" warning
    /wd4996       # Ignore "function or variable may be unsafe" warning
    ${CEF_DEBUG_INFO_FLAG}
    )
  if(USE_OFFICIAL_BUILD_SANDBOX)
    # CMake adds /RTC1, /D"_DEBUG" and a few other values by default for Debug
    # builds. We can't link the sandbox with those values so clear the CMake
    # defaults here.
    set(CMAKE_CXX_FLAGS_DEBUG "")

    list(APPEND CEF_COMPILER_FLAGS_DEBUG
      /MT           # Multithreaded release runtime
      )
  else()
    list(APPEND CEF_COMPILER_FLAGS_DEBUG
      /MTd          # Multithreaded debug runtime
      /RTC1         # Disable optimizations
      /Od           # Enable basic run-time checks
      )
  endif()
  list(APPEND CEF_COMPILER_FLAGS_RELEASE
    /MT           # Multithreaded release runtime
    /O2           # Optimize for maximum speed
    /Ob2          # Inline any suitable function
    /GF           # Enable string pooling
    )
  list(APPEND CEF_LINKER_FLAGS_DEBUG
    /DEBUG        # Generate debug information
    )
  list(APPEND CEF_EXE_LINKER_FLAGS
    /MANIFEST:NO        # No default manifest (see ADD_WINDOWS_MANIFEST macro usage)
    /LARGEADDRESSAWARE  # Allow 32-bit processes to access 3GB of RAM
    )
  list(APPEND CEF_COMPILER_DEFINES
    WIN32 _WIN32 _WINDOWS             # Windows platform
    UNICODE _UNICODE                  # Unicode build
    WINVER=0x0602 _WIN32_WINNT=0x602  # Targeting Windows 8
    NOMINMAX                          # Use the standard's templated min/max
    WIN32_LEAN_AND_MEAN               # Exclude less common API declarations
    _HAS_EXCEPTIONS=0                 # Disable exceptions
    )
  if(USE_OFFICIAL_BUILD_SANDBOX)
    list(APPEND CEF_COMPILER_DEFINES_DEBUG
      NDEBUG _NDEBUG                    # Not a debug build
      DCHECK_ALWAYS_ON=1                # DCHECKs are enabled
      )
  endif()
  list(APPEND CEF_COMPILER_DEFINES_RELEASE
    NDEBUG _NDEBUG                    # Not a debug build
    )

  # Standard libraries.
  set(CEF_STANDARD_LIBS
    comctl32.lib
    rpcrt4.lib
    shlwapi.lib
    ws2_32.lib
    )

  # CEF directory paths.
  set(CEF_RESOURCE_DIR        "${_CEF_ROOT}/Resources")
  set(CEF_BINARY_DIR          "${_CEF_ROOT}/$<CONFIGURATION>")
  set(CEF_BINARY_DIR_DEBUG    "${_CEF_ROOT}/Debug")
  set(CEF_BINARY_DIR_RELEASE  "${_CEF_ROOT}/Release")

  # CEF library paths.
  set(CEF_LIB_DEBUG   "${CEF_BINARY_DIR_DEBUG}/libcef.lib")
  set(CEF_LIB_RELEASE "${CEF_BINARY_DIR_RELEASE}/libcef.lib")

  # List of CEF binary files.
  set(CEF_BINARY_FILES
    d3dcompiler_43.dll
    d3dcompiler_47.dll
    libcef.dll
    libEGL.dll
    libGLESv2.dll
    natives_blob.bin
    snapshot_blob.bin
    )

  # List of CEF resource files.
  set(CEF_RESOURCE_FILES
    cef.pak
    cef_100_percent.pak
    cef_200_percent.pak
    cef_extensions.pak
    devtools_resources.pak
    icudtl.dat
    locales
    )

  if(USE_SANDBOX)
    list(APPEND CEF_COMPILER_DEFINES
      PSAPI_VERSION=1   # Required by cef_sandbox.lib
      CEF_USE_SANDBOX   # Used by apps to test if the sandbox is enabled
      )

    # Libraries required by cef_sandbox.lib.
    set(CEF_SANDBOX_STANDARD_LIBS
      dbghelp.lib
      psapi.lib
      version.lib
      winmm.lib
      )

    # CEF sandbox library paths.
    set(CEF_SANDBOX_LIB_DEBUG "${CEF_BINARY_DIR_DEBUG}/cef_sandbox.lib")
    set(CEF_SANDBOX_LIB_RELEASE "${CEF_BINARY_DIR_RELEASE}/cef_sandbox.lib")
  endif()

  # Configure use of ATL.
  option(USE_ATL "Enable or disable use of ATL." ON)
  if(USE_ATL)
    # Determine if the Visual Studio install supports ATL.
    get_filename_component(VC_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
    get_filename_component(VC_DIR ${VC_BIN_DIR} DIRECTORY)
    if(NOT IS_DIRECTORY "${VC_DIR}/atlmfc")
      set(USE_ATL OFF)
    endif()
  endif()

  if(USE_ATL)
    list(APPEND CEF_COMPILER_DEFINES
      CEF_USE_ATL   # Used by apps to test if ATL support is enabled
      )
  endif()
endif()
