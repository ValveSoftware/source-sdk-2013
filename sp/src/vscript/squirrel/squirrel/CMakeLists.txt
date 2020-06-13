set(SQUIRREL_SRC sqapi.cpp
                 sqbaselib.cpp
                 sqclass.cpp
                 sqcompiler.cpp
                 sqdebug.cpp
                 sqfuncstate.cpp
                 sqlexer.cpp
                 sqmem.cpp
                 sqobject.cpp
                 sqstate.cpp
                 sqtable.cpp
                 sqvm.cpp)

if(NOT DISABLE_DYNAMIC)
  add_library(squirrel SHARED ${SQUIRREL_SRC})
  add_library(squirrel::squirrel ALIAS squirrel)
  set_property(TARGET squirrel PROPERTY EXPORT_NAME squirrel)
  if(NOT SQ_DISABLE_INSTALLER)
    install(TARGETS squirrel EXPORT squirrel
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Libraries
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Libraries NAMELINK_SKIP
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Libraries
      )
    install(TARGETS squirrel EXPORT squirrel
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Development NAMELINK_ONLY
      )
  endif()
  target_include_directories(squirrel PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )
endif()

if(NOT DISABLE_STATIC)
  add_library(squirrel_static STATIC ${SQUIRREL_SRC})
  add_library(squirrel::squirrel_static ALIAS squirrel_static)
  set_property(TARGET squirrel_static PROPERTY EXPORT_NAME squirrel_static)
  if(NOT SQ_DISABLE_INSTALLER)
    install(TARGETS squirrel_static EXPORT squirrel ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Development)
  endif()
  target_include_directories(squirrel_static PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )
endif()

if(LONG_OUTPUT_NAMES)
  if(NOT DISABLE_DYNAMIC)
    set_target_properties(squirrel PROPERTIES OUTPUT_NAME squirrel3)
  endif()

  if(NOT DISABLE_STATIC)
    set_target_properties(squirrel_static PROPERTIES OUTPUT_NAME squirrel3_static)
  endif()
endif()
