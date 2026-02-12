## GLFW
if (NOT NO_X11 AND NOT FORCE_DRM AND NOT EMSCRIPTEN)
    # Configure GLFW to build only the library.
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
    set(GLFW_INSTALL OFF CACHE BOOL "")

    if(UNIX AND NOT APPLE)
        find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)
        if (NOT WAYLAND_SCANNER_EXECUTABLE)
            message(STATUS "wayland-scanner not found, disabling GLFW Wayland support")
            set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "Build support for Wayland" FORCE)
        endif()
    endif()

    add_subdirectory(glfw)

    if(APPLE)
    target_compile_options(glfw PRIVATE "-Wno-deprecated-declarations")
    endif()
    
    if(NOT DEFINED OpenGL_GL_PREFERENCE)
        # set(OpenGL_GL_PREFERENCE "LEGACY")
        set(OPENGL_GL_PREFERENCE "GLVND")
    endif()
    set(OpenGL_GL_PREFERENCE ${OPENGL_GL_PREFERENCE})
    find_package(OpenGL REQUIRED)

    message(STATUS "OPENGL_LIBRARIES: ${OPENGL_LIBRARIES}")
    message(STATUS "OPENGL_opengl_LIBRARY: ${OPENGL_opengl_LIBRARY}")
endif()

if (WIN32)
    add_subdirectory(glew)
endif (WIN32)

if (EMSCRIPTEN)
    add_subdirectory(emscripten-webxr)
endif (EMSCRIPTEN)

## Lygia
add_subdirectory(lygia)