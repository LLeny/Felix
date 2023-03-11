cmake_minimum_required( VERSION 3.6 )
enable_language( CXX )
set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

include( "helpers/cmake/glsl-shaders.cmake" )

set( BUILD_DIR ${CMAKE_SOURCE_DIR}/build )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )

if( WIN32 )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
endif()

project( felix VERSION "0.0.0" )

configure_file( "${CMAKE_CURRENT_SOURCE_DIR}/Felix/version.hpp.in" "${CMAKE_CURRENT_SOURCE_DIR}/Felix/version.hpp" )

find_package( Vulkan REQUIRED )
find_package( glfw3 REQUIRED )
find_package( Threads REQUIRED )

file(READ "libextern/vkmemalloc/src/CMakeLists.txt" FILE_CONTENTS)
string(REPLACE "CXX_STANDARD 14" "CXX_STANDARD 20" FILE_CONTENTS ${FILE_CONTENTS})
file(WRITE "libextern/vkmemalloc/src/CMakeLists.txt" ${FILE_CONTENTS})

file(READ "libextern/libwav/CMakeLists.txt" FILE_CONTENTS)
string(REPLACE "# Project Information" "add_definitions( -D__x86_64 ) #Project Information" FILE_CONTENTS ${FILE_CONTENTS})
file(WRITE "libextern/libwav/CMakeLists.txt" ${FILE_CONTENTS})

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/helpers/cmake/lua/CMakeLists.txt" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/libextern/lua" )

if( WIN32 )
    set( CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON )
endif()

if( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
    add_compile_options( "--stdlib=libc++" )
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
endif()

add_subdirectory( "libextern/fmt" )
add_subdirectory( "libextern/portaudio" )
add_subdirectory( "libextern/libwav" )
add_subdirectory( "libextern/sol2" )
add_subdirectory( "libextern/vkbootstrap" )
add_subdirectory( "libextern/vkmemalloc" )
add_subdirectory( "libextern/lua" )

if( CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" )
    add_compile_options( "/wd4716" )
elseif( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
    link_libraries( "-Wl,-rpath '-Wl,$ORIGIN'")    
endif()

add_subdirectory( "libFelix" )

file ( GLOB_RECURSE felix_SOURCES CONFIGURE_DEPENDS 
    "Felix/*.cpp" 
    "Felix/shaders/*.spv" 
    "libextern/imgui/imgui.cpp"
    "libextern/imgui/imgui_draw.cpp"
    "libextern/imgui/imgui_tables.cpp"
    "libextern/imgui/imgui_widgets.cpp"
    "libextern/imgui/backends/imgui_impl_vulkan.cpp"
    "libextern/imgui/backends/imgui_impl_glfw.cpp"
)
add_executable( ${PROJECT_NAME} ${felix_SOURCES} )

target_glsl_shaders( ${PROJECT_NAME}
       PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Felix/shaders/lynx_render.comp"
)

target_include_directories( ${PROJECT_NAME} PRIVATE 
    ${Vulkan_INCLUDE_DIRS}
    ${glfw3_INCLUDE_DIRS}
    "Felix/" 
    "libFelix/"
    "Encoder/API"
    "libextern/sol2/include"
    "libextern/portaudio/include"
    "libextern/lua"
    "libextern/vkmemalloc/include"
    "libextern/vkbootstrap/src"
    "libextern/libwav/include"
    "libextern/fmt/include"
    "libextern/imgui"
    "libextern/imgui/backends"
    "libextern/imgui-filebrowser"
    "libextern/imgui_club/imgui_memory_editor"
)

target_link_libraries( ${PROJECT_NAME} 
    Threads::Threads
    Vulkan::Vulkan
    glfw
    ${CMAKE_DL_LIBS}
    fmt
    wav::wav
    VulkanMemoryAllocator
    vk-bootstrap::vk-bootstrap
    sol2
    PortAudio
    libFelix
    lua
    )

add_custom_command(
    OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/lynx_render.comp.spv"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/Felix/shaders/lynx_render.comp.spv" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders"
    MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/Felix/shaders/lynx_render.comp.spv"
)
