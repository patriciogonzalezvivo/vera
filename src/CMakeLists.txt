# Main folders
set(INCLUDE_FOLDER "${PROJECT_SOURCE_DIR}/include")
set(SOURCE_FOLDER "${PROJECT_SOURCE_DIR}/src")
set(DEPS_FOLDER "${PROJECT_SOURCE_DIR}/deps")

# HEADERS files
file(GLOB ROOT_HEADER "${INCLUDE_FOLDER}/vera/*.h")
file(GLOB GL_HEADER "${INCLUDE_FOLDER}/vera/gl/*.h")
file(GLOB IO_HEADER "${INCLUDE_FOLDER}/vera/io/*.h")
file(GLOB OPS_HEADER "${INCLUDE_FOLDER}/vera/ops/*.h")
file(GLOB TYPES_HEADER "${INCLUDE_FOLDER}/vera/types/*.h")
file(GLOB DEVICES_HEADER "${INCLUDE_FOLDER}/vera/devices/*.h")
file(GLOB SHADERS_HEADER "${INCLUDE_FOLDER}/vera/shaders/*.h")

# SOURCE files
set(VERA_SOURCES
    ${DEPS_FOLDER}/glob/glob.cpp
    ${DEPS_FOLDER}/phonedepth/extract_depthmap.cpp
    ${DEPS_FOLDER}/skymodel/ArHosekSkyModel.cpp
    ${DEPS_FOLDER}/stb/stb_image_write.cpp
    ${DEPS_FOLDER}/stb/stb_image.cpp
    ${SOURCE_FOLDER}/app.cpp
    ${SOURCE_FOLDER}/window.cpp
    ${SOURCE_FOLDER}/gl/gl.cpp 
    ${SOURCE_FOLDER}/gl/fbo.cpp
    ${SOURCE_FOLDER}/gl/vbo.cpp
    ${SOURCE_FOLDER}/gl/shader.cpp
    ${SOURCE_FOLDER}/gl/defines.cpp 
    ${SOURCE_FOLDER}/gl/pingpong.cpp
    ${SOURCE_FOLDER}/gl/pyramid.cpp
    ${SOURCE_FOLDER}/gl/texture.cpp 
    ${SOURCE_FOLDER}/gl/textureBump.cpp
    ${SOURCE_FOLDER}/gl/textureCube.cpp 
    ${SOURCE_FOLDER}/gl/textureProps.cpp 
    ${SOURCE_FOLDER}/gl/textureStreamAV.cpp
    ${SOURCE_FOLDER}/gl/textureStreamOMX.cpp
    ${SOURCE_FOLDER}/gl/textureStreamMMAL.cpp
    ${SOURCE_FOLDER}/gl/textureStreamAudio.cpp 
    ${SOURCE_FOLDER}/gl/textureStreamSequence.cpp
    ${SOURCE_FOLDER}/gl/vertexLayout.cpp 
    ${SOURCE_FOLDER}/io/gltf.cpp
    ${SOURCE_FOLDER}/io/obj.cpp
    ${SOURCE_FOLDER}/io/ply.cpp
    ${SOURCE_FOLDER}/io/stl.cpp
    ${SOURCE_FOLDER}/ops/draw.cpp
    ${SOURCE_FOLDER}/ops/env.cpp
    ${SOURCE_FOLDER}/ops/fs.cpp 
    ${SOURCE_FOLDER}/ops/geom.cpp
    ${SOURCE_FOLDER}/ops/image.cpp
    ${SOURCE_FOLDER}/ops/intersection.cpp
    ${SOURCE_FOLDER}/ops/meshes.cpp
    ${SOURCE_FOLDER}/ops/pixel.cpp 
    ${SOURCE_FOLDER}/ops/string.cpp
    ${SOURCE_FOLDER}/ops/time.cpp
    ${SOURCE_FOLDER}/types/camera.cpp
    ${SOURCE_FOLDER}/types/font.cpp
    ${SOURCE_FOLDER}/types/image.cpp
    ${SOURCE_FOLDER}/types/line.cpp
    ${SOURCE_FOLDER}/types/light.cpp
    ${SOURCE_FOLDER}/types/label.cpp
    ${SOURCE_FOLDER}/types/material.cpp
    ${SOURCE_FOLDER}/types/mesh.cpp 
    ${SOURCE_FOLDER}/types/model.cpp
    ${SOURCE_FOLDER}/types/node.cpp
    ${SOURCE_FOLDER}/types/scene.cpp
    ${SOURCE_FOLDER}/types/triangle.cpp
    ${SOURCE_FOLDER}/devices/holoPlay.cpp 
    ${SOURCE_FOLDER}/shaders/defaultShaders.cpp
)

# C++ LIBRAY
#
add_library(vera ${VERA_SOURCES})

set_target_properties(vera PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)

find_package(BROADCOM)
if (BROADCOM_FOUND)
    message(STATUS "BROADCOM_DEFINITIONS:   ${BROADCOM_DEFINITIONS}")
    message(STATUS "BROADCOM_INCLUDE_DIRS:  ${BROADCOM_INCLUDE_DIRS}")
    message(STATUS "BROADCOM_LIBRARIES:     ${BROADCOM_LIBRARIES}")
    target_include_directories(vera PUBLIC   ${BROADCOM_INCLUDE_DIRS})
    target_link_libraries(vera PUBLIC        ${BROADCOM_LIBRARIES})
    target_compile_definitions(vera PUBLIC   ${BROADCOM_DEFINITIONS})
endif()

if (BROADCOM_INCLUDE_DIR)
message(STATUS "PLATFORM RASPBERRYPI")
target_compile_definitions(vera PUBLIC       PLATFORM_RPI)
endif()
    
find_package(ILCLIENT)
if (ILCLIENT_FOUND)
    target_include_directories(vera PUBLIC   ${ILCLIENT_INCLUDE_DIR} )
    target_link_libraries(vera PUBLIC        ${ILCLIENT_LIBRARY} )
endif()      

find_package(OMAX)
if (OMAX_FOUND AND ILCLIENT_FOUND)
    target_compile_definitions(vera PUBLIC   ${OMAX_DEFINITIONS})
    target_link_libraries(vera PUBLIC        ${OMAX_LIBRARIES})
    target_compile_definitions(vera PUBLIC   SUPPORT_OMAX)
endif()

find_package(MMAL)
if (MMAL_FOUND)
    target_link_libraries(vera PUBLIC        ${MMAL_LIBRARIES})
    target_compile_definitions(vera PUBLIC   SUPPORT_MMAL)
endif()

find_package(FFMPEG)
if (FFMPEG_FOUND)
    message(STATUS "LIBAV Supported")
    target_include_directories(vera PUBLIC ${FFMPEG_INCLUDE_DIRS})
    target_link_libraries(vera PUBLIC ${FFMPEG_LIBRARIES})
    target_compile_definitions(vera PUBLIC SUPPORT_LIBAV )
else()
    # We could not find it ourself, so let's resort to pkg_config that is better when is present
    # but may be not available in some cases (I'm looking at you windows)
    find_package(PkgConfig)
    if (PkgConfig_FOUND)
        pkg_check_modules(FFPMEG
            IMPORTED_TARGET GLOBAL
            libavdevice
            libavfilter
            libavformat
            libavcodec
            libswresample
            libswscale
            libavutil
            )
        if (FFPMEG_FOUND)
            message(STATUS "LIBAV Supported")
            target_link_libraries(vera PUBLIC PkgConfig::FFPMEG)
            target_compile_definitions(vera PUBLIC SUPPORT_LIBAV)
        endif()
    endif()
endif()

if (NO_X11 OR FORCE_GBM)
    if (BROADCOM_FOUND AND NOT FORCE_GBM)
        message(STATUS "USING BROADCOM DRIVERS")        
        target_compile_definitions(vera PUBLIC   DRIVER_BROADCOM )

        target_link_libraries(vera PUBLIC        
            ${BCM_EGL} 
            ${BCM_GLESv2}
        )

    else()
        message(STATUS "USING DRM and GBM DRIVERS")
        target_compile_definitions(vera PUBLIC   DRIVER_GBM)
        find_package(DRM)
        find_package(GBM)
        target_include_directories(vera PUBLIC
            ${DRM_INCLUDE_DIRS}
            ${GBM_INCLUDE_DIRS}
        )
        target_link_libraries(vera PUBLIC
            ${DRM_LIBRARIES}
            ${GBM_LIBRARY}
        )

        find_package(EGL)
        find_package(GLESv2)
        message(STATUS "EGL_INCLUDE_DIRS:       ${EGL_INCLUDE_DIRS}")
        message(STATUS "EGL_LIBRARIES:          ${EGL_LIBRARIES}")
        message(STATUS "GLESv2_INCLUDE_DIRS:    ${GLESv2_INCLUDE_DIRS}")
        message(STATUS "GLESv2_LIBRARIES:       ${GLESv2_LIBRARIES}")
    
        target_include_directories(vera PUBLIC
            ${EGL_INCLUDE_DIRS}
            ${GLESv2_INCLUDE_DIRS}
        )
    
        target_link_libraries(vera PUBLIC
            ${EGL_LIBRARIES}
            ${GLESv2_LIBRARIES}
        )
    endif()

else()
    message(STATUS "USING GLFW DRIVERS")

    target_compile_definitions(vera PUBLIC 
        DRIVER_GLFW 
        GL_OPENGL
    )

    target_link_libraries(vera PUBLIC 
        glfw
        ${GLFW_LIBRARIES}
    )

    # OPENGL
    if (APPLE)
        message(STATUS "PLATFORM OSX")
        target_compile_definitions(vera PUBLIC   PLATFORM_OSX)
        target_link_libraries(vera PUBLIC 
            "-framework OpenGL"
        )

    elseif (MSVC)
        message(STATUS "PLATFORM WINDOWS")
        target_compile_definitions(vera PUBLIC   PLATFORM_WINDOWS)
        target_include_directories(vera PUBLIC 
            "${PROJECT_SOURCE_DIR}/deps/glew/include"
        )

        target_link_libraries(vera PUBLIC 
            libglew_static
        )

    # RASPBERRY PI
    elseif (BROADCOM_INCLUDE_DIR)
        find_package(EGL)
        find_package(GLESv2)
        message(STATUS "EGL_INCLUDE_DIRS:       ${EGL_INCLUDE_DIRS}")
        message(STATUS "EGL_LIBRARIES:          ${EGL_LIBRARIES}")
        message(STATUS "GLESv2_INCLUDE_DIRS:    ${GLESv2_INCLUDE_DIRS}")
        message(STATUS "GLESv2_LIBRARIES:       ${GLESv2_LIBRARIES}")
    
        target_include_directories(vera PUBLIC
            ${EGL_INCLUDE_DIRS}
            ${GLESv2_INCLUDE_DIRS}
        )
    
        target_link_libraries(vera PUBLIC
            ${EGL_LIBRARIES}
            ${GLESv2_LIBRARIES}
            ${OPENGL_LIBRARIES}
        )

    elseif (EMSCRIPTEN)
        message(STATUS "PLATFORM EMSCRIPTEN")
        target_compile_definitions(vera PUBLIC   PLATFORM_EMSCRIPTEN)
        target_link_libraries(vera PUBLIC 
            ${OPENGL_LIBRARIES}
        )
        
    # #   UNCOMMENT FOR MULTIPLE THREAD SUPPORT FOR WASM
    # #   note: only works on desktop chrome 
    #     set(LFLAGS "${LFLAGS} -pthread")
    #     set(LFLAGS "${LFLAGS} -s USE_PTHREADS=1")
    #     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LFLAGS}")
    #     set_target_properties(vera PROPERTIES LINK_FLAGS "${LFLAGS}")
    #     target_link_libraries(vera PUBLIC -pthread )

    else()
        message(STATUS "PLATFORM LINUX")
        target_compile_definitions(vera PUBLIC   PLATFORM_LINUX)

        set(OpenGL_GL_PREFERENCE GLVND)
        find_package(OpenGL REQUIRED)
        target_link_libraries(vera PUBLIC 
            OpenGL::GL
        )

    endif()

endif()

# We need this directory, and users of our library will need it too
target_include_directories(vera PUBLIC 
    "${PROJECT_SOURCE_DIR}/deps/glm"
    "${PROJECT_SOURCE_DIR}/deps/stb"
    "${PROJECT_SOURCE_DIR}/deps/glob"
    "${PROJECT_SOURCE_DIR}/deps/skymodel"
    "${PROJECT_SOURCE_DIR}/deps/fontstash"
    "${PROJECT_SOURCE_DIR}/deps/miniaudio"
    "${PROJECT_SOURCE_DIR}/deps/phonedepth"
    "${PROJECT_SOURCE_DIR}/deps/tinygltf"
    "${PROJECT_SOURCE_DIR}/deps/tinyobjloader"
    "${PROJECT_SOURCE_DIR}/deps/tinyply"
    ${INCLUDE_FOLDER}
)

# IDEs should put the headers in a nice place
# source_group(   TREE ${INCLUDE_FOLDER} 
#                 PREFIX "Header Files" FILES ${ROOT_HEADER})

# install(TARGETS ${PROJECT_NAME} DESTINATION lib)
# install(FILES ${ROOT_HEADER} DESTINATION include/vera)
# install(FILES ${GL_HEADER} DESTINATION include/vera/gl)
# install(FILES ${IO_HEADER} DESTINATION include/vera/io)
# install(FILES ${OPS_HEADER} DESTINATION include/vera/ops)
# install(FILES ${TYPES_HEADER} DESTINATION include/vera/types)
# install(FILES ${DEVICES_HEADER} DESTINATION include/vera/devices)
# install(FILES ${SHADERS_HEADER} DESTINATION include/vera/shaders)

