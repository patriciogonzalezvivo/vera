cmake_minimum_required(VERSION 3.2)

project(vera CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
add_compile_options(-DGLM_FORCE_CXX14)

if (NO_X11 OR FORCE_GBM)
    add_compile_options(-std=c++14 -fpermissive -Wno-psabi)
endif()

# The compiled library code is here
add_subdirectory(deps)
add_subdirectory(src)
