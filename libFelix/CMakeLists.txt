cmake_minimum_required(VERSION 3.6)
enable_language(CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(libFelix)

find_package( OpenSSL REQUIRED )

file(GLOB SOURCE_FILES
     "*.hpp"
     "*.cpp"
)

add_library( ${PROJECT_NAME} STATIC ${SOURCE_FILES} )

target_link_libraries( ${PROJECT_NAME} OpenSSL::SSL )

target_include_directories( ${PROJECT_NAME} PRIVATE 
    "../libextern/fmt/include"
    OpenSSL::SSL
)
