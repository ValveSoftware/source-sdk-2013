# Copyright (c) 2016 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

# Must be loaded via FindCEF.cmake.
if(NOT DEFINED _CEF_ROOT_EXPLICIT)
  message(FATAL_ERROR "Use find_package(CEF) to load this file.")
endif()


#
# Shared macros.
#

# Print the current CEF configuration.
macro(PRINT_CEF_CONFIG)
  message(STATUS "*** CEF CONFIGURATION SETTINGS ***")
  message(STATUS "Generator:                    ${CMAKE_GENERATOR}")
  message(STATUS "Platform:                     ${CMAKE_SYSTEM_NAME}")
  message(STATUS "Project architecture:         ${PROJECT_ARCH}")

  if(${CMAKE_GENERATOR} STREQUAL "Ninja" OR ${CMAKE_GENERATOR} STREQUAL "Unix Makefiles")
    message(STATUS "Build type:                   ${CMAKE_BUILD_TYPE}")
  endif()

  message(STATUS "Binary distribution root:     ${_CEF_ROOT}")

  if(OS_MACOSX)
    message(STATUS "Base SDK:                     ${CMAKE_OSX_SYSROOT}")
    message(STATUS "Target SDK:                   ${CEF_TARGET_SDK}")
  endif()

  if(OS_WINDOWS)
    message(STATUS "CEF Windows sandbox:          ${USE_SANDBOX}")
    message(STATUS "Visual Studio ATL support:    ${USE_ATL}")
  endif()

  set(_libraries ${CEF_STANDARD_LIBS})
  if(OS_WINDOWS AND USE_SANDBOX)
    list(APPEND _libraries ${CEF_SANDBOX_STANDARD_LIBS})
  endif()
  message(STATUS "Standard libraries:           ${_libraries}")

  message(STATUS "Compile defines:              ${CEF_COMPILER_DEFINES}")
  message(STATUS "Compile defines (Debug):      ${CEF_COMPILER_DEFINES_DEBUG}")
  message(STATUS "Compile defines (Release):    ${CEF_COMPILER_DEFINES_RELEASE}")
  message(STATUS "C compile flags:              ${CEF_COMPILER_FLAGS} ${CEF_C_COMPILER_FLAGS}")
  message(STATUS "C compile flags (Debug):      ${CEF_COMPILER_FLAGS_DEBUG} ${CEF_C_COMPILER_FLAGS_DEBUG}")
  message(STATUS "C compile flags (Release):    ${CEF_COMPILER_FLAGS_RELEASE} ${CEF_C_COMPILER_FLAGS_RELEASE}")
  message(STATUS "C++ compile flags:            ${CEF_COMPILER_FLAGS} ${CEF_CXX_COMPILER_FLAGS}")
  message(STATUS "C++ compile flags (Debug):    ${CEF_COMPILER_FLAGS_DEBUG} ${CEF_CXX_COMPILER_FLAGS_DEBUG}")
  message(STATUS "C++ compile flags (Release):  ${CEF_COMPILER_FLAGS_RELEASE} ${CEF_CXX_COMPILER_FLAGS_RELEASE}")
  message(STATUS "Exe link flags:               ${CEF_LINKER_FLAGS} ${CEF_EXE_LINKER_FLAGS}")
  message(STATUS "Exe link flags (Debug):       ${CEF_LINKER_FLAGS_DEBUG} ${CEF_EXE_LINKER_FLAGS_DEBUG}")
  message(STATUS "Exe link flags (Release):     ${CEF_LINKER_FLAGS_RELEASE} ${CEF_EXE_LINKER_FLAGS_RELEASE}")
  message(STATUS "Shared link flags:            ${CEF_LINKER_FLAGS} ${CEF_SHARED_LINKER_FLAGS}")
  message(STATUS "Shared link flags (Debug):    ${CEF_LINKER_FLAGS_DEBUG} ${CEF_SHARED_LINKER_FLAGS_DEBUG}")
  message(STATUS "Shared link flags (Release):  ${CEF_LINKER_FLAGS_RELEASE} ${CEF_SHARED_LINKER_FLAGS_RELEASE}")

  if(OS_LINUX OR OS_WINDOWS)
    message(STATUS "CEF Binary files:             ${CEF_BINARY_FILES}")
    message(STATUS "CEF Resource files:           ${CEF_RESOURCE_FILES}")
  endif()
endmacro()

# Append platform specific sources to a list of sources.
macro(APPEND_PLATFORM_SOURCES name_of_list)
  if(OS_LINUX AND ${name_of_list}_LINUX)
    list(APPEND ${name_of_list} ${${name_of_list}_LINUX})
  endif()
  if(OS_POSIX AND ${name_of_list}_POSIX)
    list(APPEND ${name_of_list} ${${name_of_list}_POSIX})
  endif()
  if(OS_WINDOWS AND ${name_of_list}_WINDOWS)
    list(APPEND ${name_of_list} ${${name_of_list}_WINDOWS})
  endif()
  if(OS_MACOSX AND ${name_of_list}_MACOSX)
    list(APPEND ${name_of_list} ${${name_of_list}_MACOSX})
  endif()
endmacro()

# Determine the target output directory based on platform and generator.
macro(SET_CEF_TARGET_OUT_DIR)
  if(${CMAKE_GENERATOR} STREQUAL "Ninja" OR
     ${CMAKE_GENERATOR} STREQUAL "Unix Makefiles")
    # By default Ninja and Make builds don't create a subdirectory named after
    # the configuration.
    set(CEF_TARGET_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}")

    # Output binaries (executables, libraries) to the correct directory.
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CEF_TARGET_OUT_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CEF_TARGET_OUT_DIR})
  else()
    set(CEF_TARGET_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>")
  endif()
endmacro()

# Copy a list of files from one directory to another. Relative files paths are maintained.
macro(COPY_FILES target file_list source_dir target_dir)
  foreach(FILENAME ${file_list})
    set(source_file ${source_dir}/${FILENAME})
    set(target_file ${target_dir}/${FILENAME})
    if(IS_DIRECTORY ${source_file})
      add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${source_file}" "${target_file}"
        VERBATIM
        )
    else()
      add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${source_file}" "${target_file}"
        VERBATIM
        )
    endif()
  endforeach()
endmacro()

# Rename a directory replacing the target if it already exists.
macro(RENAME_DIRECTORY target source_dir target_dir)
  add_custom_command(
    TARGET ${target}
    POST_BUILD
    # Remove the target directory if it already exists.
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${target_dir}"
    # Rename the source directory to target directory.
    COMMAND ${CMAKE_COMMAND} -E rename "${source_dir}" "${target_dir}"
    VERBATIM
    )
endmacro()


#
# Linux macros.
#

if(OS_LINUX)

# Use pkg-config to find Linux libraries and update compiler/linker variables.
macro(FIND_LINUX_LIBRARIES libraries)
  # Read pkg-config info into variables.
  execute_process(COMMAND pkg-config --cflags ${libraries} OUTPUT_VARIABLE FLL_CFLAGS)
  execute_process(COMMAND pkg-config --libs-only-L --libs-only-other ${libraries} OUTPUT_VARIABLE FLL_LDFLAGS)
  execute_process(COMMAND pkg-config --libs-only-l ${libraries} OUTPUT_VARIABLE FLL_LIBS)

  # Strip leading and trailing whitepspace.
  STRING(STRIP "${FLL_CFLAGS}"  FLL_CFLAGS)
  STRING(STRIP "${FLL_LDFLAGS}" FLL_LDFLAGS)
  STRING(STRIP "${FLL_LIBS}"    FLL_LIBS)

  # Convert to a list.
  separate_arguments(FLL_CFLAGS)
  separate_arguments(FLL_LDFLAGS)
  separate_arguments(FLL_LIBS)

  # Update build variables.
  list(APPEND CEF_C_COMPILER_FLAGS    ${FLL_CFLAGS})
  list(APPEND CEF_CXX_COMPILER_FLAGS  ${FLL_CFLAGS})
  list(APPEND CEF_EXE_LINKER_FLAGS    ${FLL_LDFLAGS})
  list(APPEND CEF_SHARED_LINKER_FLAGS ${FLL_LDFLAGS})
  list(APPEND CEF_STANDARD_LIBS       ${FLL_LIBS})
endmacro()

# Set SUID permissions on the specified executable.
macro(SET_LINUX_SUID_PERMISSIONS target executable)
  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "*** Run the following command manually to set SUID permissions ***"
    COMMAND ${CMAKE_COMMAND} -E echo "EXE=\"${executable}\" && sudo -- chown root:root $EXE && sudo -- chmod 4755 $EXE"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    VERBATIM
    )
endmacro()

endif(OS_LINUX)


#
# Mac OS X macros.
#

if(OS_MACOSX)

# Fix the framework rpath in the helper executable.
macro(FIX_MACOSX_HELPER_FRAMEWORK_RPATH target)
  # The helper is in $app_name.app/Contents/Frameworks/$app_name Helper.app/Contents/MacOS/
  # so set rpath up to Contents/ so that the loader can find Frameworks/.
  set_target_properties(${target} PROPERTIES INSTALL_RPATH "@executable_path/../../../..")
  set_target_properties(${target} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
endmacro()

# Fix the framework rpath in the main executable.
macro(FIX_MACOSX_MAIN_FRAMEWORK_RPATH target)
  # The main app is at $app_name.app/Contents/MacOS/$app_name
  # so set rpath up to Contents/ so that the loader can find Frameworks/.
  set_target_properties(${target} PROPERTIES INSTALL_RPATH "@executable_path/..")
  set_target_properties(${target} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
endmacro()

# Manually process and copy over resource files.
macro(COPY_MACOSX_RESOURCES resource_list prefix_list target source_dir app_path)
  foreach(FILENAME ${resource_list})
    # Remove one or more prefixes from the source paths.
    set(TARGET_FILENAME "${FILENAME}")
    foreach(PREFIX ${prefix_list})
      string(REGEX REPLACE "^.*${PREFIX}" "" TARGET_FILENAME ${TARGET_FILENAME})
    endforeach()

    # Determine the absolute source and target paths.
    set(TARGET_PATH "${app_path}/Contents/Resources/${TARGET_FILENAME}")
    if(IS_ABSOLUTE ${FILENAME})
      set(SOURCE_PATH ${FILENAME})
    else()
      set(SOURCE_PATH "${source_dir}/${FILENAME}")
    endif()

    if(${FILENAME} MATCHES ".xib$")
      # Change the target file extension.
      string(REGEX REPLACE ".xib$" ".nib" TARGET_PATH ${TARGET_PATH})

      get_filename_component(TARGET_DIRECTORY ${TARGET_PATH} PATH)
      add_custom_command(
        TARGET ${target}
        POST_BUILD
        # Create the target directory.
        COMMAND ${CMAKE_COMMAND} -E make_directory "${TARGET_DIRECTORY}"
        # Compile the XIB file to a NIB.
        COMMAND /usr/bin/ibtool --output-format binary1 --compile "${TARGET_PATH}" "${SOURCE_PATH}"
        VERBATIM
        )
    elseif(NOT ${TARGET_FILENAME} STREQUAL "Info.plist")
      # Copy the file as-is.
      add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${SOURCE_PATH}" "${TARGET_PATH}"
        VERBATIM
        )
    endif()
  endforeach()
endmacro()

endif(OS_MACOSX)


#
# Windows macros.
#

if(OS_WINDOWS)

# Add custom manifest files to an executable target.
macro(ADD_WINDOWS_MANIFEST manifest_path target extension)
  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND "mt.exe" -nologo
            -manifest \"${manifest_path}/${target}.${extension}.manifest\" \"${manifest_path}/compatibility.manifest\"
            -outputresource:"${CEF_TARGET_OUT_DIR}/${target}.${extension}"\;\#1
    COMMENT "Adding manifest..."
    )
endmacro()

endif(OS_WINDOWS)


#
# Target configuration macros.
#

# Add a logical target that can be used to link the specified libraries into an
# executable target.
macro(ADD_LOGICAL_TARGET target debug_lib release_lib)
  add_library(${target} ${CEF_LIBTYPE} IMPORTED)
  set_target_properties(${target} PROPERTIES
    IMPORTED_LOCATION "${release_lib}"
    IMPORTED_LOCATION_DEBUG "${debug_lib}"
    IMPORTED_LOCATION_RELEASE "${release_lib}"
    )
endmacro()

# Set common target properties. Use SET_LIBRARY_TARGET_PROPERTIES() or
# SET_EXECUTABLE_TARGET_PROPERTIES() instead of calling this macro directly.
macro(SET_COMMON_TARGET_PROPERTIES target)
  # Compile flags.
  target_compile_options(${target} PUBLIC ${CEF_COMPILER_FLAGS} ${CEF_CXX_COMPILER_FLAGS})
  target_compile_options(${target} PUBLIC $<$<CONFIG:Debug>:${CEF_COMPILER_FLAGS_DEBUG} ${CEF_CXX_COMPILER_FLAGS_DEBUG}>)
  target_compile_options(${target} PUBLIC $<$<CONFIG:Release>:${CEF_COMPILER_FLAGS_RELEASE} ${CEF_CXX_COMPILER_FLAGS_RELEASE}>)

  # Compile definitions.
  target_compile_definitions(${target} PUBLIC ${CEF_COMPILER_DEFINES})
  target_compile_definitions(${target} PUBLIC $<$<CONFIG:Debug>:${CEF_COMPILER_DEFINES_DEBUG}>)
  target_compile_definitions(${target} PUBLIC $<$<CONFIG:Release>:${CEF_COMPILER_DEFINES_RELEASE}>)

  # Include directories.
  target_include_directories(${target} PUBLIC ${CEF_INCLUDE_PATH})

  # Linker flags.
  if(CEF_LINKER_FLAGS)
    string(REPLACE ";" " " _flags_str "${CEF_LINKER_FLAGS}")
    set_property(TARGET ${target} PROPERTY LINK_FLAGS ${_flags_str})
  endif()
  if(CEF_LINKER_FLAGS_DEBUG)
    string(REPLACE ";" " " _flags_str "${CEF_LINKER_FLAGS_DEBUG}")
    set_property(TARGET ${target} PROPERTY LINK_FLAGS_DEBUG ${_flags_str})
  endif()
  if(CEF_LINKER_FLAGS_RELEASE)
    string(REPLACE ";" " " _flags_str "${CEF_LINKER_FLAGS_RELEASE}")
    set_property(TARGET ${target} PROPERTY LINK_FLAGS_RELEASE ${_flags_str})
  endif()

  if(OS_MACOSX)
    # Set Xcode target properties.
    set_target_properties(${target} PROPERTIES
      XCODE_ATTRIBUTE_ALWAYS_SEARCH_USER_PATHS                    NO
      XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD                 "gnu++11"   # -std=gnu++11
      XCODE_ATTRIBUTE_CLANG_LINK_OBJC_RUNTIME                     NO          # -fno-objc-link-runtime
      XCODE_ATTRIBUTE_CLANG_WARN_OBJC_MISSING_PROPERTY_SYNTHESIS  YES         # -Wobjc-missing-property-synthesis
      XCODE_ATTRIBUTE_COPY_PHASE_STRIP                            NO
      XCODE_ATTRIBUTE_DEAD_CODE_STRIPPING[variant=Release]        YES         # -Wl,-dead_strip
      XCODE_ATTRIBUTE_GCC_C_LANGUAGE_STANDARD                     "c99"       # -std=c99
      XCODE_ATTRIBUTE_GCC_CW_ASM_SYNTAX                           NO          # No -fasm-blocks
      XCODE_ATTRIBUTE_GCC_DYNAMIC_NO_PIC                          NO
      XCODE_ATTRIBUTE_GCC_ENABLE_CPP_EXCEPTIONS                   NO          # -fno-exceptions
      XCODE_ATTRIBUTE_GCC_ENABLE_CPP_RTTI                         NO          # -fno-rtti
      XCODE_ATTRIBUTE_GCC_ENABLE_PASCAL_STRINGS                   NO          # No -mpascal-strings
      XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN              YES         # -fvisibility-inlines-hidden
      XCODE_ATTRIBUTE_GCC_OBJC_CALL_CXX_CDTORS                    YES         # -fobjc-call-cxx-cdtors
      XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN                  YES         # -fvisibility=hidden
      XCODE_ATTRIBUTE_GCC_THREADSAFE_STATICS                      NO          # -fno-threadsafe-statics
      XCODE_ATTRIBUTE_GCC_TREAT_WARNINGS_AS_ERRORS                YES         # -Werror
      XCODE_ATTRIBUTE_GCC_VERSION                                 "com.apple.compilers.llvm.clang.1_0"
      XCODE_ATTRIBUTE_GCC_WARN_ABOUT_MISSING_NEWLINE              YES         # -Wnewline-eof
      XCODE_ATTRIBUTE_USE_HEADERMAP                               NO
      OSX_ARCHITECTURES_DEBUG                                     "${CMAKE_OSX_ARCHITECTURES}"
      OSX_ARCHITECTURES_RELEASE                                   "${CMAKE_OSX_ARCHITECTURES}"
      )
  endif()
endmacro()

# Set library-specific properties.
macro(SET_LIBRARY_TARGET_PROPERTIES target)
  SET_COMMON_TARGET_PROPERTIES(${target})

  # Shared library linker flags.
  if(CEF_SHARED_LINKER_FLAGS)
    string(REPLACE ";" " " _flags_str "${CEF_SHARED_LINKER_FLAGS}")
    set_property(TARGET ${target} PROPERTY LINK_FLAGS ${_flags_str})
  endif()
  if(CEF_SHARED_LINKER_FLAGS_DEBUG)
    string(REPLACE ";" " " _flags_str "${CEF_SHARED_LINKER_FLAGS_DEBUG}")
    set_property(TARGET ${target} PROPERTY LINK_FLAGS_DEBUG ${_flags_str})
  endif()
  if(CEF_SHARED_LINKER_FLAGS_RELEASE)
    string(REPLACE ";" " " _flags_str "${CEF_SHARED_LINKER_FLAGS_RELEASE}")
    set_property(TARGET ${target} PROPERTY LINK_FLAGS_RELEASE ${_flags_str})
  endif()
endmacro()

# Set executable-specific properties.
macro(SET_EXECUTABLE_TARGET_PROPERTIES target)
  SET_COMMON_TARGET_PROPERTIES(${target})

  # Executable linker flags.
  if(CEF_EXE_LINKER_FLAGS)
    string(REPLACE ";" " " _flags_str "${CEF_EXE_LINKER_FLAGS}")
    set_property(TARGET ${target} PROPERTY LINK_FLAGS ${_flags_str})
  endif()
  if(CEF_EXE_LINKER_FLAGS_DEBUG)
    string(REPLACE ";" " " _flags_str "${CEF_EXE_LINKER_FLAGS_DEBUG}")
    set_property(TARGET ${target} PROPERTY LINK_FLAGS_DEBUG ${_flags_str})
  endif()
  if(CEF_EXE_LINKER_FLAGS_RELEASE)
    string(REPLACE ";" " " _flags_str "${CEF_EXE_LINKER_FLAGS_RELEASE}")
    set_property(TARGET ${target} PROPERTY LINK_FLAGS_RELEASE ${_flags_str})
  endif()
endmacro()
