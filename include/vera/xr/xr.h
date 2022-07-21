#pragma once

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
// #define GLFW_INCLUDE_ES3

#include <glm/gtc/type_ptr.hpp>
#include "webxr.h"
#endif

#include "../types/camera.h"

namespace vera {

enum XrMode {
    NONE_XR_MODE = 0,
    VR_MODE = 1,
    AR_MODE = 2
};

void    setXR(XrMode _mode);
XrMode  getXR();

#if defined(__EMSCRIPTEN__)
void requestXR(XrMode _mode); 
#endif

}