#pragma once

#include <string>
#include <functional>

#include "gl/gl.h"
#include "glm/glm.hpp"

namespace vera {

enum WindowStyle {
    HEADLESS = 0,
    REGULAR,
    ALLWAYS_ON_TOP,
    UNDECORATED,
    UNDECORATED_ALLWAYS_ON_TOP,
    FULLSCREEN,
    LENTICULAR,
    EMBEDDED
};

struct WindowProperties {
    WindowStyle style   = REGULAR;
    size_t      major   = 2;
    size_t      minor   = 0;
    size_t      msaa    = 0;
    
    std::string vendor = "";
    std::string renderer = "";
    std::string version = "";
    std::string glsl = "";
    std::string extensions = "";

    #if defined(DRIVER_DRM) 
    std::string display = "";
    char        mode[DRM_DISPLAY_MODE_LEN] = "";
    #endif

    #if !defined(DRIVER_GLFW)
    std::string mouse = "/dev/input/mice";
    #endif

    #if defined(__EMSCRIPTEN__)
    size_t      webgl = 0;
    #endif

    int         screen_x = -1;
    int         screen_y = -1;
    int         screen_width = -1;
    int         screen_height = -1;
};

//  GL Context
//----------------------------------------------
int         initGL(WindowProperties _properties = WindowProperties());
bool        isGL();
void        updateGL();
void        renderGL();
void        closeGL();

std::string getVendor();
std::string getRenderer();
std::string getGLVersion();
std::string getGLSLVersion();
std::string getExtensions();
bool        haveExtension(std::string _name);

#if defined(__EMSCRIPTEN__)
size_t      getWebGLVersionNumber();
#elif defined(PLATFORM_RPI)
EGLDisplay  getEGLDisplay();
EGLContext  getEGLContext();
#endif

//  Windows/Viewport
//----------------------------------------------
void        updateViewport();

void        setFps(int _fps);
void        setPixelDensity(float _density);
void        setViewport(float _width, float _height);
void        setWindowSize(int _width, int _height);
void        setWindowTitle(const char* _title);
void        setWindowVSync(bool _value);
void        setWindowIcon(unsigned char* _data, size_t _width, size_t _height);

bool        isFullscreen();
void        setFullscreen(bool _fullscreen);

int         getScreenWidth();
int         getScreenHeight();
float       getPixelDensity(bool _compute = false);

const glm::ivec4& getViewport();
const glm::mat4&  getOrthoMatrix();
const glm::mat4&  getFlippedOrthoMatrix();

int         getWindowWidth();
int         getWindowHeight();
int         getWindowMSAA();
WindowStyle getWindowStyle();


// TIME
// ---------------------------------------
glm::vec4   getDate();
double      getTimeSec();
double      getTime();
double      getDelta();
double      getFps();
float       getRestSec();
int         getRestMs();
int         getRestUs();

// Mouse Keyboards
// ---------------------------------------
void        setMousePosition(float _x, float _y);
void        setMouseVisibility(bool _visible);

float       getMouseX();
float       getMouseY();

float       getMouseVelX();
float       getMouseVelY();
int         getMouseButton();
bool        getMouseEntered();

bool        isShiftPressed();
bool        isControlPressed();

// EVENTS CALLBACKS
//----------------------------------------------
void    setViewportResizeCallback(std::function<void(int,int)>);
void    setKeyPressCallback(std::function<void(int)>);
void    setMouseMoveCallback(std::function<void(float, float)>);
void    setMousePressCallback(std::function<void(float, float, int)>);
void    setMouseReleaseCallback(std::function<void(float, float, int)>);
void    setMouseDragCallback(std::function<void(float, float, int)>);
void    setScrollCallback(std::function<void(float)>);
void    setDropCallback(std::function<void(int, const char**)>);

}