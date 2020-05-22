set(SQSTDLIB_SRC sqstdaux.cpp
                 sqstdblob.cpp
                 sqstdio.cpp
                 sqstdmath.cpp
                 sqstdrex.cpp
                 sqstdstream.cpp
                 sqstdstring.cpp
                 sqstdsystem.cpp)

if(NOT DISABLE_DYNAMIC)
  add_library(sqstdlib SHARED ${SQSTDLIB_SRC})
  add_library(squirrel::sqstdlib ALIAS sqstdlib)
  set_property(TARGET sqstdlib PROPERTY EXPORT_NAME sqstdlib)
  target_link_libraries(sqstdlib squirrel)
  if(NOT SQ_DISABLE_INSTALLER)
    install(TARGETS sqstdlib EXPORT squirrel
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Libraries
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Libraries NAMELINK_SKIP
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Libraries
      )
    install(TARGETS sqstdlib EXPORT squirrel
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Development NAMELINK_ONLY
      )
  endif()
  target_include_directories(sqstdlib PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )
endif()

if(NOT DISABLE_STATIC)
  add_library(sqstdlib_static STATIC ${SQSTDLIB_SRC})
  add_library(squirrel::sqstdlib_static ALIAS sqstdlib_static)
  set_property(TARGET sqstdlib_static PROPERTY EXPORT_NAME sqstdlib_static)
  if(NOT SQ_DISABLE_INSTALLER)
    install(TARGETS sqstdlib_static EXPORT squirrel ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Development)
  endif()
  target_include_directories(sqstdlib_static PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )
endif()

if(LONG_OUTPUT_NAMES)
  if(NOT DISABLE_DYNAMIC)
    set_target_properties(sqstdlib PROPERTIES OUTPUT_NAME sqstdlib3)
  endif()

  if(NOT DISABLE_STATIC)
    set_target_properties(sqstdlib_static PROPERTIES OUTPUT_NAME sqstdlib3_static)
  endif()
endif()
