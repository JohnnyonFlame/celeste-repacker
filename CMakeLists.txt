cmake_minimum_required(VERSION 3.15)
project(CelesteRepack)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ISA_NEON "Set to ON to enable NEON instruction set" ${ODROIDN2})
if (ISA_NEON)
    set(ISA_NEON ON)
    set(ASTC_ENC_TARGET astcenc-neon-static)
else()
    set(ASTC_ENC_TARGET astcenc-native-static)
endif()

set(CLI OFF)
add_subdirectory(3rdparty/astc-encoder)

file(GLOB SOURCES src/*.cpp)

add_executable(celeste-repacker ${SOURCES})
target_link_libraries(celeste-repacker PRIVATE pthread ${ASTC_ENC_TARGET})
target_include_directories(celeste-repacker PUBLIC 3rdparty/astc-encoder/Source)