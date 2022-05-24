cmake_minimum_required(VERSION 3.15)
project(CelesteRepack)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CLI OFF)
add_subdirectory(3rdparty/astc-encoder)

file(GLOB SOURCES src/*.cpp)

add_executable(celeste-repacker ${SOURCES})
target_link_libraries(celeste-repacker PRIVATE pthread astcenc-native-static)
target_include_directories(celeste-repacker PUBLIC 3rdparty/astc-encoder/Source)