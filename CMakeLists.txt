cmake_minimum_required(VERSION 3.10)

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
