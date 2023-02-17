# Install script for directory: /home/patricio/Desktop/glslViewer/deps/vera/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/patricio/Desktop/glslViewer/deps/vera/build/src/libvera.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/vera" TYPE FILE FILES
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/app.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/window.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/vera/gl" TYPE FILE FILES
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/gl/cubemapFace.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/gl/defines.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/gl/fbo.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/gl/gl.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/gl/pingpong.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/gl/pyramid.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/gl/shader.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/gl/texture.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/gl/textureBump.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/gl/textureCube.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/gl/textureProps.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/gl/textureStream.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/gl/textureStreamAV.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/gl/textureStreamAudio.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/gl/textureStreamMMAL.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/gl/textureStreamOMX.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/gl/textureStreamSequence.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/gl/vbo.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/gl/vertexLayout.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/vera/io" TYPE FILE FILES
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/io/gltf.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/io/obj.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/io/ply.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/io/stl.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/vera/ops" TYPE FILE FILES
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/ops/draw.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/ops/env.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/ops/fs.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/ops/geom.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/ops/image.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/ops/intersection.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/ops/math.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/ops/meshes.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/ops/pixel.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/ops/string.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/ops/time.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/vera/shaders" TYPE FILE FILES
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/shaders/billboard.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/shaders/cubemap.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/shaders/default.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/shaders/defaultShaders.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/shaders/default_buffers.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/shaders/default_error.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/shaders/default_scene.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/shaders/draw.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/shaders/dynamic_billboard.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/shaders/fxaa.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/shaders/light_ui.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/shaders/plot.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/shaders/poissonfill.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/vera/types" TYPE FILE FILES
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/types/boundingBox.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/types/bvh.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/types/camera.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/types/font.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/types/image.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/types/label.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/types/light.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/types/line.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/types/material.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/types/mesh.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/types/model.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/types/node.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/types/plane.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/types/props.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/types/ray.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/types/scene.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/types/sky.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/types/triangle.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/vera/xr" TYPE FILE FILES
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/xr/holoPlay.h"
    "/home/patricio/Desktop/glslViewer/deps/vera/include/vera/xr/xr.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/usr/local/lib/python3.10/dist-packages/vera/_vera.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/local/lib/python3.10/dist-packages/vera/_vera.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/usr/local/lib/python3.10/dist-packages/vera/_vera.so"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/lib/python3.10/dist-packages/vera/_vera.so")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/usr/local/lib/python3.10/dist-packages/vera" TYPE MODULE FILES "/home/patricio/Desktop/glslViewer/deps/vera/build/src/_vera.so")
  if(EXISTS "$ENV{DESTDIR}/usr/local/lib/python3.10/dist-packages/vera/_vera.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/local/lib/python3.10/dist-packages/vera/_vera.so")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/usr/local/lib/python3.10/dist-packages/vera/_vera.so")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/lib/python3.10/dist-packages/vera/__init__.py")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/usr/local/lib/python3.10/dist-packages/vera" TYPE FILE FILES "/home/patricio/Desktop/glslViewer/deps/vera/__init__.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/lib/python3.10/dist-packages/vera/vera.py")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/usr/local/lib/python3.10/dist-packages/vera" TYPE FILE FILES "/home/patricio/Desktop/glslViewer/deps/vera/build/src/vera.py")
endif()

