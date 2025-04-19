#pragma once

#include <string>
#include <functional>

#include "gl/gl.h"
#include "glm/glm.hpp"

#ifndef VERA_KEY_SPACE
#define VERA_KEY_SPACE              32
#define VERA_KEY_APOSTROPHE         39  /* ' */
#define VERA_KEY_COMMA              44  /* , */
#define VERA_KEY_MINUS              45  /* - */
#define VERA_KEY_PERIOD             46  /* . */
#define VERA_KEY_SLASH              47  /* / */
#define VERA_KEY_0                  48
#define VERA_KEY_1                  49
#define VERA_KEY_2                  50
#define VERA_KEY_3                  51
#define VERA_KEY_4                  52
#define VERA_KEY_5                  53
#define VERA_KEY_6                  54
#define VERA_KEY_7                  55
#define VERA_KEY_8                  56
#define VERA_KEY_9                  57
#define VERA_KEY_SEMICOLON          59  /* ; */
#define VERA_KEY_EQUAL              61  /* = */
#define VERA_KEY_A                  65
#define VERA_KEY_B                  66
#define VERA_KEY_C                  67
#define VERA_KEY_D                  68
#define VERA_KEY_E                  69
#define VERA_KEY_F                  70
#define VERA_KEY_G                  71
#define VERA_KEY_H                  72
#define VERA_KEY_I                  73
#define VERA_KEY_J                  74
#define VERA_KEY_K                  75
#define VERA_KEY_L                  76
#define VERA_KEY_M                  77
#define VERA_KEY_N                  78
#define VERA_KEY_O                  79
#define VERA_KEY_P                  80
#define VERA_KEY_Q                  81
#define VERA_KEY_R                  82
#define VERA_KEY_S                  83
#define VERA_KEY_T                  84
#define VERA_KEY_U                  85
#define VERA_KEY_V                  86
#define VERA_KEY_W                  87
#define VERA_KEY_X                  88
#define VERA_KEY_Y                  89
#define VERA_KEY_Z                  90
#define VERA_KEY_LEFT_BRACKET       91  /* [ */
#define VERA_KEY_BACKSLASH          92  /* \ */
#define VERA_KEY_RIGHT_BRACKET      93  /* ] */
#define VERA_KEY_GRAVE_ACCENT       96  /* ` */
#define VERA_KEY_WORLD_1            161 /* non-US #1 */
#define VERA_KEY_WORLD_2            162 /* non-US #2 */

/* Function keys */
#define VERA_KEY_ESCAPE             256
#define VERA_KEY_ENTER              257
#define VERA_KEY_TAB                258
#define VERA_KEY_BACKSPACE          259
#define VERA_KEY_INSERT             260
#define VERA_KEY_DELETE             261
#define VERA_KEY_RIGHT              262
#define VERA_KEY_LEFT               263
#define VERA_KEY_DOWN               264
#define VERA_KEY_UP                 265
#define VERA_KEY_PAGE_UP            266
#define VERA_KEY_PAGE_DOWN          267
#define VERA_KEY_HOME               268
#define VERA_KEY_END                269
#define VERA_KEY_CAPS_LOCK          280
#define VERA_KEY_SCROLL_LOCK        281
#define VERA_KEY_NUM_LOCK           282
#define VERA_KEY_PRINT_SCREEN       283
#define VERA_KEY_PAUSE              284
#define VERA_KEY_F1                 290
#define VERA_KEY_F2                 291
#define VERA_KEY_F3                 292
#define VERA_KEY_F4                 293
#define VERA_KEY_F5                 294
#define VERA_KEY_F6                 295
#define VERA_KEY_F7                 296
#define VERA_KEY_F8                 297
#define VERA_KEY_F9                 298
#define VERA_KEY_F10                299
#define VERA_KEY_F11                300
#define VERA_KEY_F12                301
#define VERA_KEY_F13                302
#define VERA_KEY_F14                303
#define VERA_KEY_F15                304
#define VERA_KEY_F16                305
#define VERA_KEY_F17                306
#define VERA_KEY_F18                307
#define VERA_KEY_F19                308
#define VERA_KEY_F20                309
#define VERA_KEY_F21                310
#define VERA_KEY_F22                311
#define VERA_KEY_F23                312
#define VERA_KEY_F24                313
#define VERA_KEY_F25                314
#define VERA_KEY_KP_0               320
#define VERA_KEY_KP_1               321
#define VERA_KEY_KP_2               322
#define VERA_KEY_KP_3               323
#define VERA_KEY_KP_4               324
#define VERA_KEY_KP_5               325
#define VERA_KEY_KP_6               326
#define VERA_KEY_KP_7               327
#define VERA_KEY_KP_8               328
#define VERA_KEY_KP_9               329
#define VERA_KEY_KP_DECIMAL         330
#define VERA_KEY_KP_DIVIDE          331
#define VERA_KEY_KP_MULTIPLY        332
#define VERA_KEY_KP_SUBTRACT        333
#define VERA_KEY_KP_ADD             334
#define VERA_KEY_KP_ENTER           335
#define VERA_KEY_KP_EQUAL           336
#define VERA_KEY_LEFT_SHIFT         340
#define VERA_KEY_LEFT_CONTROL       341
#define VERA_KEY_LEFT_ALT           342
#define VERA_KEY_LEFT_SUPER         343
#define VERA_KEY_RIGHT_SHIFT        344
#define VERA_KEY_RIGHT_CONTROL      345
#define VERA_KEY_RIGHT_ALT          346
#define VERA_KEY_RIGHT_SUPER        347
#define VERA_KEY_MENU               348
#endif

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
    char        mode[DRM_DISPLAY_MODE_LEN] = "\0";
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
int                 initGL(WindowProperties _properties = WindowProperties());
const bool          isGL();
void                updateGL();
void                renderGL();
void                closeGL();

const std::string&  getVendor();
const std::string&  getRenderer();
const std::string&  getGLVersion();
const std::string&  getGLSLVersion();
const std::string&  getExtensions();
const bool          haveExtension(const std::string& _name);

#if defined(__EMSCRIPTEN__)
const size_t        getWebGLVersionNumber();
#elif defined(PLATFORM_RPI)
const EGLDisplay    getEGLDisplay();
const EGLContext    getEGLContext();
#endif

//  Windows/Viewport
//----------------------------------------------
void                updateViewport();

void                setFps(int _fps);
void                setPixelDensity(float _density);
void                setViewport(float _width, float _height);
void                setWindowSize(int _width, int _height);
void                setWindowTitle(const char* _title);
void                setWindowVSync(bool _value);
void                setWindowIcon(unsigned char* _data, size_t _width, size_t _height);

const bool          isFullscreen();
void                setFullscreen(bool _fullscreen);

const int           getScreenWidth();
const int           getScreenHeight();
const float         getPixelDensity(bool _compute = false);

const glm::ivec4&   getViewport();
const glm::mat4&    getOrthoMatrix();
const glm::mat4&    getFlippedOrthoMatrix();

const int           getWindowWidth();
const int           getWindowHeight();
const int           getWindowMSAA();
const WindowStyle   getWindowStyle();


// TIME
// ---------------------------------------
glm::vec4           getDate();
const double        getTimeSec();
const double        getTime();
const double        getDelta();
const double        getFps();
const float         getRestSec();
const int           getRestMs();
const int           getRestUs();

// Mouse / Keyboards
// ---------------------------------------
void                setMousePosition(float _x, float _y);
void                setMouseVisibility(bool _visible);

const glm::vec2&    getMousePosition();
const glm::vec2     getMousePositionFlipped();
const float         getMouseX();
const float         getMouseY();
const float         getMouseYFlipped();

const glm::vec2&    getMouseVel();
const glm::vec2     getMouseVelFlipped();
const float         getMouseVelX();
const float         getMouseVelY();
const float         getMouseVelYFlipped();

const int           getMouseButton();
const bool          getMouseEntered();

const bool          isShiftPressed();
const bool          isControlPressed();

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