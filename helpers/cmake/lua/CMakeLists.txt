project( lua  VERSION 1.0.0 )
include(GNUInstallDirs)
enable_language( CXX )
set( CMAKE_ENABLE_EXPORTS ON )

cmake_minimum_required( VERSION 3.2 )

# remove warnings
add_definitions( -D_CRT_SECURE_NO_WARNINGS ) 

add_library ( lua SHARED "onelua.c" )

set_target_properties( lua PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER lua.h)

target_include_directories( lua PRIVATE . )

install( TARGETS lua
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )