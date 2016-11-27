# Copyright (c) 2014 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

#
# Source files.
#

# cefclient browser sources.
set(CEFCLIENT_BROWSER_BROWSER_SRCS
  browser/binding_test.cc
  browser/binding_test.h
  browser/browser_window.cc
  browser/browser_window.h
  browser/bytes_write_handler.cc
  browser/bytes_write_handler.h
  browser/client_app_browser.cc
  browser/client_app_browser.h
  browser/client_app_delegates_browser.cc
  browser/client_handler.cc
  browser/client_handler.h
  browser/client_handler_osr.cc
  browser/client_handler_osr.h
  browser/client_handler_std.cc
  browser/client_handler_std.h
  browser/client_types.h
  browser/dialog_test.cc
  browser/dialog_test.h
  browser/drm_test.cc
  browser/drm_test.h
  browser/geometry_util.cc
  browser/geometry_util.h
  browser/main_context.cc
  browser/main_context.h
  browser/main_context_impl.cc
  browser/main_context_impl.h
  browser/main_message_loop.cc
  browser/main_message_loop.h
  browser/main_message_loop_external_pump.cc
  browser/main_message_loop_external_pump.h
  browser/main_message_loop_std.cc
  browser/main_message_loop_std.h
  browser/osr_dragdrop_events.h
  browser/osr_renderer.cc
  browser/osr_renderer.h
  browser/preferences_test.cc
  browser/preferences_test.h
  browser/resource.h
  browser/resource_util.cc
  browser/resource_util.h
  browser/response_filter_test.cc
  browser/response_filter_test.h
  browser/root_window.cc
  browser/root_window.h
  browser/root_window_create.cc
  browser/root_window_manager.cc
  browser/root_window_manager.h
  browser/scheme_test.cc
  browser/scheme_test.h
  browser/temp_window.h
  browser/test_runner.cc
  browser/test_runner.h
  browser/urlrequest_test.cc
  browser/urlrequest_test.h
  browser/window_test.cc
  browser/window_test.h
  browser/window_test_runner.cc
  browser/window_test_runner.h
  )
source_group(cefclient\\\\browser FILES ${CEFCLIENT_BROWSER_BROWSER_SRCS})

set(CEFCLIENT_BROWSER_SRCS
  ${CEFCLIENT_BROWSER_BROWSER_SRCS}
  )

# cefclient common sources.
set(CEFCLIENT_COMMON_COMMON_SRCS
  common/client_app.cc
  common/client_app.h
  common/client_app_delegates_common.cc
  common/client_app_other.cc
  common/client_app_other.h
  common/client_switches.cc
  common/client_switches.h
  common/scheme_test_common.cc
  common/scheme_test_common.h
  )
source_group(cefclient\\\\common FILES ${CEFCLIENT_COMMON_COMMON_SRCS})

set(CEFCLIENT_COMMON_SRCS
  ${CEFCLIENT_COMMON_COMMON_SRCS}
  )

# cefclient renderer sources.
set(CEFCLIENT_RENDERER_RENDERER_SRCS
  renderer/client_app_delegates_renderer.cc
  renderer/client_app_renderer.cc
  renderer/client_app_renderer.h
  renderer/client_renderer.cc
  renderer/client_renderer.h
  renderer/performance_test.cc
  renderer/performance_test.h
  renderer/performance_test_setup.h
  renderer/performance_test_tests.cc
  )
source_group(cefclient\\\\renderer FILES ${CEFCLIENT_RENDERER_RENDERER_SRCS})

set(CEFCLIENT_RENDERER_SRCS
  ${CEFCLIENT_RENDERER_RENDERER_SRCS}
  )

#cefclient Linux sources
set(CEFCLIENT_LINUX_SRCS
  cefclient_gtk.cc
  )
source_group(cefclient FILES ${CEFCLIENT_LINUX_SRCS})

set(CEFCLIENT_LINUX_BROWSER_SRCS
  browser/browser_window_osr_gtk.cc
  browser/browser_window_osr_gtk.h
  browser/browser_window_std_gtk.cc
  browser/browser_window_std_gtk.h
  browser/dialog_handler_gtk.cc
  browser/dialog_handler_gtk.h
  browser/main_context_impl_posix.cc
  browser/main_message_loop_external_pump_linux.cc
  browser/print_handler_gtk.cc
  browser/print_handler_gtk.h
  browser/resource_util_linux.cc
  browser/resource_util_posix.cc
  browser/root_window_gtk.cc
  browser/root_window_gtk.h
  browser/root_window_views.cc
  browser/root_window_views.h
  browser/temp_window_x11.cc
  browser/temp_window_x11.h
  browser/views_window.cc
  browser/views_window.h
  browser/window_test_runner_gtk.cc
  browser/window_test_runner_gtk.h
  browser/window_test_runner_views.cc
  browser/window_test_runner_views.h
  )
source_group(cefclient\\\\browser FILES ${CEFCLIENT_LINUX_BROWSER_SRCS})

set(CEFCLIENT_LINUX_SRCS
  ${CEFCLIENT_LINUX_SRCS}
  ${CEFCLIENT_LINUX_BROWSER_SRCS}
  )

#cefclient Mac OS X sources
set(CEFCLIENT_MACOSX_SRCS
  cefclient_mac.mm
  )
source_group(cefclient FILES ${CEFCLIENT_MACOSX_SRCS})

set(CEFCLIENT_MACOSX_BROWSER_SRCS
  browser/browser_window_osr_mac.h
  browser/browser_window_osr_mac.mm
  browser/browser_window_std_mac.h
  browser/browser_window_std_mac.mm
  browser/main_context_impl_posix.cc
  browser/main_message_loop_external_pump_mac.mm
  browser/resource_util_mac.mm
  browser/resource_util_posix.cc
  browser/root_window_mac.h
  browser/root_window_mac.mm
  browser/temp_window_mac.h
  browser/temp_window_mac.mm
  browser/window_test_runner_mac.h
  browser/window_test_runner_mac.mm
  )
source_group(cefclient\\\\browser FILES ${CEFCLIENT_MACOSX_BROWSER_SRCS})

set(CEFCLIENT_MACOSX_SRCS
  ${CEFCLIENT_MACOSX_SRCS}
  ${CEFCLIENT_MACOSX_BROWSER_SRCS}
  )

# cefclient Mac OS X helper sources.
set(CEFCLIENT_HELPER_SRCS
  process_helper_mac.cc
  )
source_group(cefclient FILES ${CEFCLIENT_HELPER_SRCS})

set(CEFCLIENT_MACOSX_HELPER_SRCS
  ${CEFCLIENT_HELPER_SRCS}
  )

#cefclient Windows sources
set(CEFCLIENT_WINDOWS_SRCS
  cefclient_win.cc
  )
source_group(cefclient FILES ${CEFCLIENT_WINDOWS_SRCS})

set(CEFCLIENT_WINDOWS_BROWSER_SRCS
  browser/browser_window_osr_win.cc
  browser/browser_window_osr_win.h
  browser/browser_window_std_win.cc
  browser/browser_window_std_win.h
  browser/main_context_impl_win.cc
  browser/main_message_loop_external_pump_win.cc
  browser/main_message_loop_multithreaded_win.cc
  browser/main_message_loop_multithreaded_win.h
  browser/osr_dragdrop_win.cc
  browser/osr_dragdrop_win.h
  browser/osr_window_win.cc
  browser/osr_window_win.h
  browser/resource_util_win.cc
  browser/root_window_views.cc
  browser/root_window_views.h
  browser/root_window_win.cc
  browser/root_window_win.h
  browser/temp_window_win.cc
  browser/temp_window_win.h
  browser/util_win.cc
  browser/util_win.h
  browser/views_window.cc
  browser/views_window.h
  browser/window_test_runner_views.cc
  browser/window_test_runner_views.h
  browser/window_test_runner_win.cc
  browser/window_test_runner_win.h
  )
source_group(cefclient\\\\browser FILES ${CEFCLIENT_WINDOWS_BROWSER_SRCS})

set(CEFCLIENT_WINDOWS_RESOURCES_WIN_SRCS
  resources/win/cefclient.exe.manifest
  resources/win/cefclient.ico
  resources/win/cefclient.rc
  resources/win/small.ico
  )
source_group(cefclient\\\\resources\\\\win FILES ${CEFCLIENT_WINDOWS_RESOURCES_WIN_SRCS})

set(CEFCLIENT_WINDOWS_SRCS
  ${CEFCLIENT_WINDOWS_SRCS}
  ${CEFCLIENT_WINDOWS_BROWSER_SRCS}
  ${CEFCLIENT_WINDOWS_RESOURCES_WIN_SRCS}
  )

# cefclient resources.
set(CEFCLIENT_RESOURCES_RESOURCES_SRCS
  resources/binding.html
  resources/dialogs.html
  resources/draggable.html
  resources/drm.html
  resources/localstorage.html
  resources/logo.png
  resources/menu_icon.1x.png
  resources/menu_icon.2x.png
  resources/osr_test.html
  resources/other_tests.html
  resources/pdf.html
  resources/pdf.pdf
  resources/performance.html
  resources/performance2.html
  resources/preferences.html
  resources/response_filter.html
  resources/transparency.html
  resources/urlrequest.html
  resources/window.html
  resources/window_icon.1x.png
  resources/window_icon.2x.png
  resources/xmlhttprequest.html
  )
source_group(cefclient\\\\resources FILES ${CEFCLIENT_RESOURCES_RESOURCES_SRCS})

set(CEFCLIENT_RESOURCES_RESOURCES_MAC_SRCS_MACOSX
  resources/mac/Info.plist
  resources/mac/cefclient.icns
  )
APPEND_PLATFORM_SOURCES(CEFCLIENT_RESOURCES_RESOURCES_MAC_SRCS)
source_group(cefclient\\\\resources\\\\mac FILES ${CEFCLIENT_RESOURCES_RESOURCES_MAC_SRCS})

set(CEFCLIENT_RESOURCES_RESOURCES_MAC_ENGLISH_LPROJ_SRCS_MACOSX
  resources/mac/English.lproj/InfoPlist.strings
  resources/mac/English.lproj/MainMenu.xib
  )
APPEND_PLATFORM_SOURCES(CEFCLIENT_RESOURCES_RESOURCES_MAC_ENGLISH_LPROJ_SRCS)
source_group(cefclient\\\\resources\\\\mac\\\\English.lproj FILES ${CEFCLIENT_RESOURCES_RESOURCES_MAC_ENGLISH_LPROJ_SRCS})

set(CEFCLIENT_RESOURCES_SRCS
  ${CEFCLIENT_RESOURCES_RESOURCES_SRCS}
  ${CEFCLIENT_RESOURCES_RESOURCES_MAC_SRCS}
  ${CEFCLIENT_RESOURCES_RESOURCES_MAC_ENGLISH_LPROJ_SRCS}
  )


#
# Shared configuration.
#

# Target executable names.
set(CEF_TARGET "cefclient")
if(OS_MACOSX)
  set(CEF_HELPER_TARGET "cefclient Helper")
endif()

# Logical target used to link the libcef library.
ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")

# Determine the target output directory.
SET_CEF_TARGET_OUT_DIR()


#
# Linux configuration.
#

if(OS_LINUX)
  # All sources required by the "cefclient" target. Generates an executable that
  # is used for all processes.
  set(CEFCLIENT_SRCS
    ${CEFCLIENT_BROWSER_SRCS}
    ${CEFCLIENT_COMMON_SRCS}
    ${CEFCLIENT_RENDERER_SRCS}
    ${CEFCLIENT_RESOURCES_SRCS}
    ${CEFCLIENT_LINUX_SRCS}
    )

  # Find required libraries and update compiler/linker variables.
  FIND_LINUX_LIBRARIES("gmodule-2.0 gtk+-2.0 gthread-2.0 gtk+-unix-print-2.0 gtkglext-1.0")

  # Executable target.
  add_executable(${CEF_TARGET} ${CEFCLIENT_SRCS})
  SET_EXECUTABLE_TARGET_PROPERTIES(${CEF_TARGET})
  add_dependencies(${CEF_TARGET} libcef_dll_wrapper)
  target_link_libraries(${CEF_TARGET} libcef_lib libcef_dll_wrapper ${CEF_STANDARD_LIBS})

  # Set rpath so that libraries can be placed next to the executable.
  set_target_properties(${CEF_TARGET} PROPERTIES INSTALL_RPATH "$ORIGIN")
  set_target_properties(${CEF_TARGET} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
  set_target_properties(${CEF_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CEF_TARGET_OUT_DIR})

  # Copy CEF binary and resource files to the target output directory.
  COPY_FILES("${CEF_TARGET}" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
  COPY_FILES("${CEF_TARGET}" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")

  # Copy cefclient resource files to the target output directory.
  COPY_FILES("${CEF_TARGET}" "${CEFCLIENT_RESOURCES_SRCS}" "${CMAKE_CURRENT_SOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")
  # Rename the "res" directory to "files".
  RENAME_DIRECTORY("${CEF_TARGET}" "${CEF_TARGET_OUT_DIR}/resources" "${CEF_TARGET_OUT_DIR}/files")

  # Set SUID permissions on the chrome-sandbox target.
  SET_LINUX_SUID_PERMISSIONS("${CEF_TARGET}" "${CEF_TARGET_OUT_DIR}/chrome-sandbox")
endif()


#
# Mac OS X configuration.
#

if(OS_MACOSX)
  # All sources required by the "cefclient" target. Generates an app bundle that
  # is used only for the browser process.
  set(CEFCLIENT_SRCS
    ${CEFCLIENT_BROWSER_SRCS}
    ${CEFCLIENT_COMMON_SRCS}
    ${CEFCLIENT_RESOURCES_SRCS}
    ${CEFCLIENT_MACOSX_SRCS}
    )

  # All sources required by the "cefclient Helper" target. Generates an app
  # bundle that is used only for non-browser processes.
  set(CEFCLIENT_HELPER_SRCS
    ${CEFCLIENT_COMMON_SRCS}
    ${CEFCLIENT_RENDERER_SRCS}
    ${CEFCLIENT_MACOSX_HELPER_SRCS}
    )

  # Output paths for the app bundles.
  set(CEF_APP "${CEF_TARGET_OUT_DIR}/${CEF_TARGET}.app")
  set(CEF_HELPER_APP "${CEF_TARGET_OUT_DIR}/${CEF_HELPER_TARGET}.app")

  # Variable referenced from Info.plist files.
  set(PRODUCT_NAME "${CEF_TARGET}")

  # Helper executable target.
  add_executable(${CEF_HELPER_TARGET} MACOSX_BUNDLE ${CEFCLIENT_HELPER_SRCS})
  SET_EXECUTABLE_TARGET_PROPERTIES(${CEF_HELPER_TARGET})
  add_dependencies(${CEF_HELPER_TARGET} libcef_dll_wrapper)
  target_link_libraries(${CEF_HELPER_TARGET} libcef_lib libcef_dll_wrapper ${CEF_STANDARD_LIBS})
  set_target_properties(${CEF_HELPER_TARGET} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/resources/mac/helper-Info.plist
    )

  # Fix the framework rpath in the helper executable.
  FIX_MACOSX_HELPER_FRAMEWORK_RPATH(${CEF_HELPER_TARGET})

  # Main executable target.
  add_executable(${CEF_TARGET} MACOSX_BUNDLE ${CEFCLIENT_RESOURCES_SRCS} ${CEFCLIENT_SRCS})
  SET_EXECUTABLE_TARGET_PROPERTIES(${CEF_TARGET})
  add_dependencies(${CEF_TARGET} libcef_dll_wrapper "${CEF_HELPER_TARGET}")
  target_link_libraries(${CEF_TARGET} libcef_lib libcef_dll_wrapper ${CEF_STANDARD_LIBS} "-framework OpenGL")
  set_target_properties(${CEF_TARGET} PROPERTIES
    RESOURCE "${CEFCLIENT_RESOURCES_SRCS}"
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/resources/mac/Info.plist
    )

  # Copy files into the main app bundle.
  add_custom_command(
    TARGET ${CEF_TARGET}
    POST_BUILD
    # Copy the helper app bundle into the Frameworks directory.
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CEF_HELPER_APP}"
            "${CEF_APP}/Contents/Frameworks/${CEF_HELPER_TARGET}.app"
    # Copy the CEF framework into the Frameworks directory.
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CEF_BINARY_DIR}/Chromium Embedded Framework.framework"
            "${CEF_APP}/Contents/Frameworks/Chromium Embedded Framework.framework"
    VERBATIM
    )

  # Fix the framework rpath in the main executable.
  FIX_MACOSX_MAIN_FRAMEWORK_RPATH(${CEF_TARGET})

  if(NOT ${CMAKE_GENERATOR} STREQUAL "Xcode")
    # Manually process and copy over resource files.
    # The Xcode generator handles this via the set_target_properties RESOURCE directive.
    set(PREFIXES "resources/mac/" "resources/")  # Remove these prefixes from input file paths.
    COPY_MACOSX_RESOURCES("${CEFCLIENT_RESOURCES_SRCS}" "${PREFIXES}" "${CEF_TARGET}" "${CMAKE_CURRENT_SOURCE_DIR}" "${CEF_APP}")
  endif()
endif()


#
# Windows configuration.
#

if(OS_WINDOWS)
  # All sources required by the "cefclient" target. Generates an executable that
  # is used for all processes.
  set(CEFCLIENT_SRCS
    ${CEFCLIENT_BROWSER_SRCS}
    ${CEFCLIENT_COMMON_SRCS}
    ${CEFCLIENT_RENDERER_SRCS}
    ${CEFCLIENT_RESOURCES_SRCS}
    ${CEFCLIENT_WINDOWS_SRCS}
    )

  # Executable target.
  add_executable(${CEF_TARGET} WIN32 ${CEFCLIENT_SRCS})
  SET_EXECUTABLE_TARGET_PROPERTIES(${CEF_TARGET})
  add_dependencies(${CEF_TARGET} libcef_dll_wrapper)
  target_link_libraries(${CEF_TARGET} libcef_lib libcef_dll_wrapper ${CEF_STANDARD_LIBS} glu32.lib opengl32.lib)

  if(USE_SANDBOX)
    # Logical target used to link the cef_sandbox library.
    ADD_LOGICAL_TARGET("cef_sandbox_lib" "${CEF_SANDBOX_LIB_DEBUG}" "${CEF_SANDBOX_LIB_RELEASE}")
    target_link_libraries(${CEF_TARGET} cef_sandbox_lib ${CEF_SANDBOX_STANDARD_LIBS})
  endif()

  # Add the custom manifest files to the executable.
  ADD_WINDOWS_MANIFEST("${CMAKE_CURRENT_SOURCE_DIR}/resources/win" "${CEF_TARGET}" "exe")

  # Copy CEF binary and resource files to the target output directory.
  COPY_FILES("${CEF_TARGET}" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
  COPY_FILES("${CEF_TARGET}" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")
endif()
