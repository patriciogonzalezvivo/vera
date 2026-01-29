cmake_minimum_required(VERSION 3.10)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

project(vera C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
add_compile_options(-DGLM_FORCE_CXX11 -fPIE -fPIC)

if (NO_X11 OR FORCE_DRM)
    add_compile_options(-std=c++11 -fpermissive -Wno-psabi)
endif()

# The compiled library code is here
add_subdirectory(deps)
add_subdirectory(src)
