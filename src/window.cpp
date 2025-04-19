#include "vera/window.h"

#include <time.h>
#include <string>
#include <cstring>
#include <fstream>
#include <iostream>
#include <algorithm>

#if defined(_WIN32)
    #define NOMINMAX
    #include <windows.h>
#else
    #include <sys/time.h>
#endif 

#include "vera/ops/fs.h"
#include "vera/ops/time.h"
#include "vera/ops/string.h"
#include "glm/gtc/matrix_transform.hpp"

// Common global variables
//----------------------------------------------------
static glm::ivec4               viewport;
static glm::ivec4               viewport_last;
static vera::WindowProperties   properties;
struct timespec                 time_start;
static glm::mat4                orthoMatrix;
static glm::mat4                orthoFlippedMatrix;

typedef struct {
    glm::vec2   pos;
    glm::vec2   vel;
    glm::vec2   drag;
    int         button;
    bool        entered;
} Mouse;
static Mouse            mouse;

struct timeval          tv;
static uint32_t         screen_width = 0;
static uint32_t         screen_height = 0;
static double           elapseTime = 0.0;
static double           delta = 0.0;
static double           FPS = 0.0;
static double           restSec = 0.0167; // default 60fps 
static float            fPixelDensity = 1.0f;
static float            yScroll = 0.0f;

static bool             bShift = false;    
static bool             bControl = false;

#if defined(DRIVER_GLFW)

#if defined(__APPLE__)
    #define GL_PROGRAM_BINARY_LENGTH 0x8741
    #include <GLFW/glfw3.h>
    #include <OpenGL/gl.h>
    #include <OpenGL/glext.h>

#elif defined(_WIN32)
    #include <GL/glew.h>
    #include "GLFW/glfw3.h"
    #define APIENTRY __stdcall

#elif defined(__EMSCRIPTEN__)
    #include <emscripten.h>
    #include <emscripten/html5.h>
    #define GL_GLEXT_PROTOTYPES
    #define EGL_EGLEXT_PROTOTYPES
    #include <GLFW/glfw3.h>

#else
    // ANY LINUX using GLFW 
    #define GL_GLEXT_PROTOTYPES
    #include "GLFW/glfw3.h"
#endif

namespace vera {
//----------------------------------------------------
static bool             left_mouse_button_down = false;
static GLFWwindow*      window;

#if defined(PLATFORM_RPI)
#include "GLFW/glfw3native.h"
EGLDisplay getEGLDisplay() { return glfwGetEGLDisplay(); }
EGLContext getEGLContext() { return glfwGetEGLContext(window); }
#endif

typedef struct _rect { int x, y; int w, h; } _rect;
int _min(int a, int b) {
    return a < b ? a : b;
}

int _max(int a, int b) {
    return a > b ? a : b;
}


#define RECT_INTERSECTS(ra, rb) \
    ((ra)->x <= ((rb)->x + (rb)->w) && ((ra)->x + (ra)->w) >= (rb)->x && \
    (ra)->y <= ((rb)->y + (rb)->h) && ((ra)->y + (ra)->h) >= (rb)->y)

static _rect _get_intersection(_rect* ra, _rect* rb) {
    _rect result = { 0, 0, 0, 0 };
    if (RECT_INTERSECTS(ra, rb)) {
        result.x = _max(ra->x, rb->x);
        result.w = _min((ra->x + ra->w), (rb->x + rb->w)) - result.x;
        result.y = _max(ra->y, rb->y);
        result.h = _min((ra->y + ra->h), (rb->y + rb->h)) - result.y;
    }
    return result;
}

GLFWAPI GLFWmonitor* getMonitorFromWindow(GLFWwindow* window) {  
    GLFWmonitor* result = NULL;

    int monitorCount;
    GLFWmonitor** monitors;
    const GLFWvidmode* vidmode;

    unsigned int currentDim, overlapDim;
    int overlapMonitor, i;

    _rect windowRect;
    _rect monitorRect;
    _rect scratchRect = { 0, 0, 0, 0 };
    _rect overlapRect = { 0, 0, 0, 0 };

    assert(window != NULL);

    // _GLFW_REQUIRE_INIT_OR_RETURN(NULL);

    monitors = glfwGetMonitors(&monitorCount);

    if (monitorCount == 1)
        result = monitors[0];

    else if (monitorCount > 1) {
        glfwGetWindowPos(window, &windowRect.x, &windowRect.y);
        glfwGetWindowSize(window, &windowRect.w, &windowRect.h);

        glfwGetWindowFrameSize(window, &scratchRect.x, &scratchRect.y, 
        &scratchRect.w, &scratchRect.h);

        windowRect.x -= scratchRect.x;
        windowRect.y -= scratchRect.y;
        windowRect.w += scratchRect.x + scratchRect.w;
        windowRect.h += scratchRect.y + scratchRect.h;

        overlapMonitor = -1;

        for (i = 0; i < monitorCount; i++) {
            glfwGetMonitorPos(monitors[i], &monitorRect.x, &monitorRect.y);

            vidmode = glfwGetVideoMode(monitors[i]);
            monitorRect.w = vidmode->width;
            monitorRect.h = vidmode->height;

            scratchRect = _get_intersection(&windowRect, &monitorRect);

            currentDim = scratchRect.w * scratchRect.h;
            overlapDim = overlapRect.w * overlapRect.h;

            if (currentDim > 0 && currentDim > overlapDim) {
                overlapRect = scratchRect;
                overlapMonitor = i;
            }
        }

        if (overlapMonitor >= 0)
        result = monitors[overlapMonitor];
    }

    return result;
}

#else

#if defined(DRIVER_BROADCOM)
    #include "bcm_host.h"
    #undef countof
#elif defined(DRIVER_GBM)
    #include <xf86drm.h>
    #include <xf86drmMode.h>
    #include <gbm.h>
#endif

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <assert.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h> 
#include <unistd.h>

namespace vera {

#define check() assert(glGetError() == 0)

// EGL context globals
EGLDisplay display;
EGLContext context;
EGLSurface surface;

const EGLDisplay getEGLDisplay() { return display; }
const EGLContext getEGLContext() { return context; }

// Get the EGL error back as a string. Useful for debugging.
static const char *eglGetErrorStr() {
    switch (eglGetError()) {
    case EGL_SUCCESS:
        return "The last function succeeded without error.";
    case EGL_NOT_INITIALIZED:
        return "EGL is not initialized, or could not be initialized, for the "
               "specified EGL display connection.";
    case EGL_BAD_ACCESS:
        return "EGL cannot access a requested resource (for example a context "
               "is bound in another thread).";
    case EGL_BAD_ALLOC:
        return "EGL failed to allocate resources for the requested operation.";
    case EGL_BAD_ATTRIBUTE:
        return "An unrecognized attribute or attribute value was passed in the "
               "attribute list.";
    case EGL_BAD_CONTEXT:
        return "An EGLContext argument does not name a valid EGL rendering "
               "context.";
    case EGL_BAD_CONFIG:
        return "An EGLConfig argument does not name a valid EGL frame buffer "
               "configuration.";
    case EGL_BAD_CURRENT_SURFACE:
        return "The current surface of the calling thread is a window, pixel "
               "buffer or pixmap that is no longer valid.";
    case EGL_BAD_DISPLAY:
        return "An EGLDisplay argument does not name a valid EGL display "
               "connection.";
    case EGL_BAD_SURFACE:
        return "An EGLSurface argument does not name a valid surface (window, "
               "pixel buffer or pixmap) configured for GL rendering.";
    case EGL_BAD_MATCH:
        return "Arguments are inconsistent (for example, a valid context "
               "requires buffers not supplied by a valid surface).";
    case EGL_BAD_PARAMETER:
        return "One or more argument values are invalid.";
    case EGL_BAD_NATIVE_PIXMAP:
        return "A NativePixmapType argument does not refer to a valid native "
               "pixmap.";
    case EGL_BAD_NATIVE_WINDOW:
        return "A NativeWindowType argument does not refer to a valid native "
               "window.";
    case EGL_CONTEXT_LOST:
        return "A power management event has occurred. The application must "
               "destroy all contexts and reinitialise OpenGL ES state and "
               "objects to continue rendering.";
    default:
        break;
    }
    return "Unknown error!";
}
#endif

std::function<void(int,int)>            onViewportResize;
std::function<void(int)>                onKeyPress;
std::function<void(float, float)>       onMouseMove;
std::function<void(float, float, int)>  onMousePress;
std::function<void(float, float, int)>  onMouseDrag;
std::function<void(float, float, int)>  onMouseRelease;
std::function<void(float)>              onScroll;
std::function<void(int, const char**)>  onDrop;

void setViewportResizeCallback(std::function<void(int,int)> _callback) { onViewportResize = _callback; }
void setKeyPressCallback(std::function<void(int)> _callback) { onKeyPress = _callback; }
void setMouseMoveCallback(std::function<void(float, float)> _callback) { onMouseMove = _callback; }
void setMousePressCallback(std::function<void(float, float, int)> _callback) { onMousePress = _callback; }
void setMouseDragCallback(std::function<void(float, float, int)> _callback) { onMouseDrag = _callback; }
void setMouseReleaseCallback(std::function<void(float, float, int)> _callback) { onMouseRelease = _callback; }
void setScrollCallback(std::function<void(float)>_callback) { onScroll = _callback; }
void setDropCallback(std::function<void(int, const char**)>_callback) { onDrop = _callback; }

#if defined(DRIVER_BROADCOM)
    DISPMANX_DISPLAY_HANDLE_T dispman_display;

#elif defined(DRIVER_GBM)
    // https://github.com/matusnovak/rpi-opengl-without-x/blob/master/triangle_rpi4.c

    int device;
    drmModeModeInfo mode;
    struct gbm_device *gbmDevice;
    struct gbm_surface *gbmSurface;
    drmModeCrtc *crtc;
    uint32_t connectorId;

    static drmModeConnector *getConnector(drmModeRes *resources) {
        for (int i = 0; i < resources->count_connectors; i++) {
            drmModeConnector *connector = drmModeGetConnector(device, resources->connectors[i]);
            if (connector->connection == DRM_MODE_CONNECTED)
                return connector;
            drmModeFreeConnector(connector);
        }
        return NULL;
    }

    static drmModeEncoder *findEncoder(drmModeRes *resources, drmModeConnector *connector) {
        if (connector->encoder_id)
            return drmModeGetEncoder(device, connector->encoder_id);
        return NULL;
    }

    static struct gbm_bo *previousBo = NULL;
    static uint32_t previousFb;

    static void gbmSwapBuffers() {
        struct gbm_bo *bo = gbm_surface_lock_front_buffer(gbmSurface);
        uint32_t handle = gbm_bo_get_handle(bo).u32;
        uint32_t pitch = gbm_bo_get_stride(bo);
        uint32_t fb;
        drmModeAddFB(device, mode.hdisplay, mode.vdisplay, 24, 32, pitch, handle, &fb);
        drmModeSetCrtc(device, crtc->crtc_id, fb, 0, 0, &connectorId, 1, &mode);
        if (previousBo) {
            drmModeRmFB(device, previousFb);
            gbm_surface_release_buffer(gbmSurface, previousBo);
        }
        previousBo = bo;
        previousFb = fb;
    }

    static void gbmClean() {
        // set the previous crtc
        drmModeSetCrtc(device, crtc->crtc_id, crtc->buffer_id, crtc->x, crtc->y, &connectorId, 1, &crtc->mode);
        drmModeFreeCrtc(crtc);
        if (previousBo) {
            drmModeRmFB(device, previousFb);
            gbm_surface_release_buffer(gbmSurface, previousBo);
        }
        gbm_surface_destroy(gbmSurface);
        gbm_device_destroy(gbmDevice);
    }
#endif

#if !defined(DRIVER_GLFW)
    static bool bHostInited = false;
    static void initHost() {
        if (bHostInited)
            return;

        #if defined(DRIVER_BROADCOM)
            bcm_host_init();

        #elif defined(DRIVER_GBM)
            if (!urlExists(properties.display))
                std::cout << "Can't open display " <<  properties.display << " seams it doesn't exist" << std::endl;
            
            device = open(  properties.display.c_str(), O_RDWR | O_CLOEXEC);

            drmModeRes *resources = drmModeGetResources(device);
            if (resources == NULL) {
                std::cerr << "Unable to get DRM resources" << std::endl;
                return EXIT_FAILURE;
            }

            drmModeConnector *connector = getConnector(resources);
            if (connector == NULL) {
                std::cerr << "Unable to get connector" << std::endl;
                drmModeFreeResources(resources);
                return EXIT_FAILURE;
            }

            connectorId = connector->connector_id;
            mode = connector->modes[0];

            drmModeEncoder *encoder = findEncoder(resources, connector);
            if (connector == NULL) {
                std::cerr << "Unable to get encoder" << std::endl;
                drmModeFreeConnector(connector);
                drmModeFreeResources(resources);
                return EXIT_FAILURE;
            }

            crtc = drmModeGetCrtc(device, encoder->crtc_id);
            drmModeFreeEncoder(encoder);
            drmModeFreeConnector(connector);
            drmModeFreeResources(resources);
            gbmDevice = gbm_create_device(device);
            gbmSurface = gbm_surface_create(gbmDevice, mode.hdisplay, mode.vdisplay, GBM_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
        #endif

        bHostInited = true;
    }

    static EGLDisplay getDisplay() {
        initHost();
        // printf("resolution: %ix%i\n", mode.hdisplay, mode.vdisplay);

        #if defined(DRIVER_BROADCOM)
            return eglGetDisplay(EGL_DEFAULT_DISPLAY);

        #elif defined(DRIVER_GBM)

            return eglGetDisplay(gbmDevice);
        #endif

        return nullptr;
    }
#endif

#if defined(__EMSCRIPTEN__)

    static void update_canvas_size() {
        double width, height;
        emscripten_get_element_css_size("#canvas", &width, &height);
        width *= emscripten_get_device_pixel_ratio();
        height *= emscripten_get_device_pixel_ratio();
        setWindowSize(width, height);
    } 

    static EM_BOOL on_canvassize_changed(int eventType, const void *reserved, void *userData) {
        update_canvas_size();
        return EM_FALSE;
    }

    bool enable_extension(const char* name) {
        auto ctx = emscripten_webgl_get_current_context();
        return emscripten_webgl_enable_extension(ctx, name);
    }

    static EM_BOOL resize_callback(int eventType, const EmscriptenUiEvent *e, void* userData) {
        update_canvas_size();
        return EM_TRUE;
    }
#endif

#if defined(PLATFORM_WINDOWS)
    const int CLOCK_MONOTONIC = 0;
    int clock_gettime(int, struct timespec* spec) {
        __int64 wintime; GetSystemTimeAsFileTime((FILETIME*)&wintime);
        wintime -= 116444736000000000i64;  //1jan1601 to 1jan1970
        spec->tv_sec = wintime / 10000000i64;           //seconds
        spec->tv_nsec = wintime % 10000000i64 * 100;      //nano-seconds
        return 0;
    }
#endif

int initGL(WindowProperties _prop) {
    clock_gettime(CLOCK_MONOTONIC, &time_start);
    properties = _prop;

    if (properties.style == EMBEDDED) {
        setViewport(properties.screen_width, properties.screen_height);
        return 0;
    }

// NON GLFW
#if !defined(DRIVER_GLFW)

    if (properties.screen_x == -1)
        properties.screen_x = 0;

    if (properties.screen_y == -1)
        properties.screen_y = 0;

    if (properties.screen_width == -1 || properties.style == FULLSCREEN)
        properties.screen_width = getScreenWidth();

    if (properties.screen_height == -1 || properties.style == FULLSCREEN)
        properties.screen_height = getScreenHeight();

    // Clear application state
    EGLBoolean result;

    display = getDisplay();
    assert(display != EGL_NO_DISPLAY);
    check();

    result = eglInitialize(display, NULL, NULL);
    #if defined(DRIVER_GBM)
    if (EGL_FALSE == result) {
        std::cout << "Fail to initialize EGL context from display " << properties.display << " try another." << std::endl;
    }
    #endif
    assert(EGL_FALSE != result);
    check();

    // Make sure that we can use OpenGL in this EGL app.
    // result = eglBindAPI(EGL_OPENGL_API);
    result = eglBindAPI(EGL_OPENGL_ES_API);
    assert(EGL_FALSE != result);
    check();
    
    static const EGLint configAttribs[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_SAMPLE_BUFFERS, 1, EGL_SAMPLES, 4,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };

    static const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    EGLConfig config;
    EGLint numConfigs;

    // get an appropriate EGL frame buffer configuration
    if (eglChooseConfig(display, configAttribs, &config, 1, &numConfigs) == EGL_FALSE) {
        std::cerr << "Failed to get EGL configs! Error: " << eglGetErrorStr() << std::endl;
        eglTerminate(display);
        #if defined(DRIVER_GBM)
        gbmClean();
        #endif
        return EXIT_FAILURE;
    }

    // create an EGL rendering context
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        std::cerr << "Failed to create EGL context! Error: " << eglGetErrorStr() << std::endl;
        eglTerminate(display);
        #if defined(DRIVER_GBM)
        gbmClean();
        #endif
        return EXIT_FAILURE;
    }

    #if defined(DRIVER_BROADCOM)
        static EGL_DISPMANX_WINDOW_T nativeviewport;

        VC_RECT_T dst_rect;
        VC_RECT_T src_rect;

        //  Initially the viewport is for all the screen
        dst_rect.x = properties.screen_x;
        dst_rect.y = properties.screen_y;
        dst_rect.width = properties.screen_width;
        dst_rect.height = properties.screen_height;

        src_rect.x = 0;
        src_rect.y = 0;
        src_rect.width = properties.screen_width << 16;
        src_rect.height = properties.screen_height << 16;

        DISPMANX_ELEMENT_HANDLE_T dispman_element;
        DISPMANX_UPDATE_HANDLE_T dispman_update;

        if (properties.style == HEADLESS) {
            uint32_t dest_image_handle;
            DISPMANX_RESOURCE_HANDLE_T dispman_resource;
            dispman_resource = vc_dispmanx_resource_create(VC_IMAGE_RGBA32, properties.screen_width, properties.screen_height, &dest_image_handle);
            dispman_display = vc_dispmanx_display_open_offscreen(dispman_resource, DISPMANX_NO_ROTATE);
        } 
        else
            dispman_display = vc_dispmanx_display_open(0); // LCD

        // VC_DISPMANX_ALPHA_T alpha = { 
        //     DISPMANX_FLAGS_ALPHA_FROM_SOURCE | DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS, 
        //     0, //255, 
        //     0 
        // };

        dispman_update = vc_dispmanx_update_start(0);
        dispman_element = vc_dispmanx_element_add(  dispman_update, dispman_display,
                                                    0/*layer*/, &dst_rect, 0/*src*/,
                                                    &src_rect, DISPMANX_PROTECTION_NONE,
                                                    0 /*&alpha*/ , 0/*clamp*/, (DISPMANX_TRANSFORM_T)0/*transform*/);

        nativeviewport.element = dispman_element;
        nativeviewport.width = properties.screen_width;
        nativeviewport.height = properties.screen_height;
        vc_dispmanx_update_submit_sync( dispman_update );
        check();

        surface = eglCreateWindowSurface( display, config, &nativeviewport, NULL );
        assert(surface != EGL_NO_SURFACE);
        check();

    #elif defined(DRIVER_GBM)
        surface = eglCreateWindowSurface(display, config, gbmSurface, NULL);
        if (surface == EGL_NO_SURFACE) {
            std::cerr << "Failed to create EGL surface! Error: " << eglGetErrorStr() << std::endl;
            eglDestroyContext(display, context);
            eglTerminate(display);
            gbmClean();
            return EXIT_FAILURE;
        }
    #endif

    // connect the context to the surface
    result = eglMakeCurrent(display, surface, surface, context);
    assert(EGL_FALSE != result);
    check();

// GLFW
#else

    if (properties.screen_width == -1)
        properties.screen_width = 512;

    if (properties.screen_height == -1)
        properties.screen_height = 512;

    glfwSetErrorCallback([](int err, const char* msg)->void {
        std::cerr << "GLFW error 0x"<<std::hex<<err<<std::dec<<": "<<msg<<"\n";
    });
    
    if(!glfwInit()) {
        std::cerr << "ABORT: GLFW init failed" << std::endl;
        exit(-1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, properties.major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, properties.minor);
    if (properties.major >= 3 && properties.minor >= 2) {
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    }

    if (properties.msaa != 0)
        glfwWindowHint(GLFW_SAMPLES, properties.msaa);

    if (properties.style == HEADLESS)
        glfwWindowHint(GLFW_VISIBLE, GL_FALSE);

    else if (properties.style == UNDECORATED )
        glfwWindowHint(GLFW_DECORATED, GL_FALSE);
        
    else if (properties.style == ALLWAYS_ON_TOP )
        glfwWindowHint(GLFW_FLOATING, GL_TRUE);

    else if (properties.style == UNDECORATED_ALLWAYS_ON_TOP) {
        glfwWindowHint(GLFW_DECORATED, GL_FALSE);
        glfwWindowHint(GLFW_FLOATING, GL_TRUE);
    }

    if (properties.style == FULLSCREEN) {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        properties.screen_width = mode->width;
        properties.screen_height = mode->height;
        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
        window = glfwCreateWindow(properties.screen_width, properties.screen_height, "", monitor, NULL);
    }
    else if (properties.style == LENTICULAR) {
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
        glfwWindowHint(GLFW_DECORATED, GL_FALSE);

        int count = 0;
        int monitorID = 1;
        GLFWmonitor **monitors = glfwGetMonitors(&count);

        if (count > 2) { //if we have more than 2 screens, try and find the looking glass screen ID 
            monitorID = -1;
            for (int i = 0; i < count; i++){
                const char* name = glfwGetMonitorName(monitors[i]);
                if (name && strlen(name) >= 3) // if monitor name is > than 3 chars
                    if(name[0] == 'L' && name[1] == 'K' && name[2] == 'G') // found a match for the looking glass screen
                        monitorID = i;
            }
            if (monitorID == -1) //could not find the looking glass screen
                monitorID = 1;
        }
        
        if (count > 1) {
            const GLFWvidmode* mode = glfwGetVideoMode(monitors[monitorID]);
            properties.screen_width = mode->width;
            properties.screen_height = mode->height;

            int xpos, ypos;
            glfwGetMonitorPos(monitors[monitorID], &xpos, &ypos);
            window = glfwCreateWindow(properties.screen_width, properties.screen_height, "", NULL, NULL);
            
            if (properties.screen_x != 0 || properties.screen_y != 0) {
                properties.screen_x += xpos;
                properties.screen_y += ypos;
            }
        }
        else {
            window = glfwCreateWindow(properties.screen_width, properties.screen_height, "", NULL, NULL);
        }

        glfwSetWindowPos(window, properties.screen_x, properties.screen_y);
    }
    else {
        window = glfwCreateWindow(properties.screen_width, properties.screen_height, "", NULL, NULL);

        #if !defined(__EMSCRIPTEN__)
        if (properties.screen_x == -1)
            properties.screen_x = getScreenWidth() / 2 - properties.screen_width / 2;

        if (properties.screen_y == -1)
            properties.screen_y = getScreenHeight() / 2 - properties.screen_height / 2;

        glfwSetWindowPos(window, properties.screen_x, properties.screen_y);
        #endif
    }

    if (!window) {
        glfwTerminate();
        std::cerr << "ABORT: GLFW create window failed" << std::endl;
        exit(-1);
    }

    glfwMakeContextCurrent(window);
    #if defined(_WIN32)
        glewInit();
    #endif

    glfwSetCharCallback(window, [](GLFWwindow* _window, unsigned int _key) {
        if (onKeyPress)
            onKeyPress(_key);
    });

    glfwSetKeyCallback(window, [](GLFWwindow* _window, int _key, int _scancode, int _action, int _mods) {
        if (_action == GLFW_PRESS && (_key == GLFW_KEY_LEFT_SHIFT || GLFW_KEY_RIGHT_SHIFT) )
            bShift = true;
        else if (_action == GLFW_RELEASE && (_key == GLFW_KEY_LEFT_SHIFT || GLFW_KEY_RIGHT_SHIFT) )
            bShift = false;

        else if (_action == GLFW_PRESS && (_key == GLFW_KEY_LEFT_CONTROL || GLFW_KEY_RIGHT_CONTROL) )
            bControl = true;
        else if (_action == GLFW_RELEASE && (_key == GLFW_KEY_LEFT_CONTROL || GLFW_KEY_RIGHT_CONTROL) )
            bControl = false;

        if (_key == GLFW_KEY_ESCAPE ||
            _key == GLFW_KEY_RIGHT || _key == GLFW_KEY_LEFT || _key == GLFW_KEY_UP || _key == GLFW_KEY_DOWN) {
            if (onKeyPress)
                onKeyPress(_key);
        }
    });

    glfwSetScrollCallback(window, [](GLFWwindow* _window, double xoffset, double yoffset) {
        if (onScroll)
            onScroll(-yoffset * fPixelDensity);
    });

    glfwSetDropCallback(window, [](GLFWwindow* window, int count, const char** paths) {
        if (onDrop)
            onDrop(count, paths);
    });

    glfwSetWindowContentScaleCallback(window, [](GLFWwindow* _window, float xscale, float yscale) {
        fPixelDensity = (xscale > yscale ? xscale : yscale);
        std::cout << "Pixel Density: " << fPixelDensity << std::endl;

        if (onViewportResize)
            onViewportResize(properties.screen_width, properties.screen_height);
    });

    // callback when the mouse cursor enters/leaves
    glfwSetCursorEnterCallback(window, [](GLFWwindow* _window, int entered) {
        mouse.entered = (bool)entered;
    });

    //  // callback when a mouse button is pressed or released
    //  glfwSetMouseButtonCallback(window, [](GLFWwindow* _window, int button, int action, int mods) {
    //     if (button == GLFW_MOUSE_BUTTON_1) {
    //         if (action == GLFW_PRESS && !left_mouse_button_down)
    //             left_mouse_button_down = true;
    //         else if (action == GLFW_RELEASE && left_mouse_button_down)
    //             left_mouse_button_down = false;
    //     }
    //     if (action == GLFW_PRESS) {
    //         mouse.drag.x = mouse.pos.x;
    //         mouse.drag.y = mouse.pos.y;
    //     }
    // });

    glfwSetMouseButtonCallback(window, [](GLFWwindow* _window, int _button, int _action, int _mods) {        
        mouse.button = _button;

        if (_button == GLFW_MOUSE_BUTTON_1) {
            if (_action == GLFW_PRESS && !left_mouse_button_down)
                left_mouse_button_down = true;
            else if (_action == GLFW_RELEASE && left_mouse_button_down)
                left_mouse_button_down = false;
        }

        if (_action == GLFW_PRESS) {
            mouse.drag = mouse.pos;

            if (onMousePress)
                onMousePress(mouse.pos.x, mouse.pos.y, mouse.button);
        }
        else {
            if (onMouseRelease)
                onMouseRelease(mouse.pos.x, mouse.pos.y, mouse.button);
        }
    });	

    // callback when the mouse cursor moves
    glfwSetCursorPosCallback(window, [](GLFWwindow* _window, double x, double y) {
        // mouse.pos.x,mouse.pos.y is the current cursor position, constrained
        // mouse.vel.x,mouse.vel.y is the distance the mouse cursor has moved
        // since the last callback, during a drag gesture.
        // mouse.drag is the previous mouse position, during a drag gesture.
        // Note that mouse.drag is *not* constrained to the viewport.
        mouse.vel.x = x - mouse.drag.x;
        mouse.vel.y = y - mouse.drag.y;
        mouse.drag.x = x;
        mouse.drag.y = y;

        // mouse.pos.x,mouse.pos.y is the current cursor position, constrained
        // to the viewport.
        mouse.pos.x = x;
        mouse.pos.y = y;
        if (mouse.pos.x < 0) mouse.pos.x = 0;
        if (mouse.pos.y < 0) mouse.pos.y = 0;
        if (mouse.pos.x > getWindowWidth()) mouse.pos.x = getWindowWidth();
        if (mouse.pos.y > getWindowHeight()) mouse.pos.y = getWindowHeight();

        /*
            * TODO: the following code would best be moved into the
            * mouse button callback. If you click the mouse button without
            * moving the mouse, then using this code, the mouse click doesn't
            * register until the cursor is moved. (@doug-moen)
            */
        int action1 = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1);
        int action2 = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2);
        int button = 0;

        if (action1 == GLFW_PRESS) button = 1;
        else if (action2 == GLFW_PRESS) button = 2;

        // Lunch events
        if (mouse.button == 0 && button != mouse.button) {
            mouse.button = button;
            if (onMousePress)
                onMousePress(mouse.pos.x, mouse.pos.y, mouse.button);
        }
        else {
            mouse.button = button;
        }

        if (mouse.vel.x != 0.0 || mouse.vel.y != 0.0) {
            if (button != 0) {
                if (onMouseDrag)
                    onMouseDrag(mouse.pos.x, mouse.pos.y, mouse.button);
            }
            else {
                if (onMouseMove) 
                    onMouseMove(mouse.pos.x, mouse.pos.y);
            }
        }
    });

#if defined(__EMSCRIPTEN__)
    enable_extension("OES_texture_float");
    enable_extension("OES_texture_half_float");
    enable_extension("OES_standard_derivatives");
    enable_extension("OES_texture_float_linear");
    enable_extension("OES_texture_half_float_linear");
    enable_extension("EXT_color_buffer_float");

    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, true, resize_callback);
#else

    glfwSetWindowPosCallback(window, [](GLFWwindow* _window, int x, int y) {
        if (fPixelDensity != getPixelDensity(true))
            updateViewport();
    });

    glfwSetWindowSizeCallback(window, [](GLFWwindow* _window, int _w, int _h) {
        setViewport(_w,_h);
    });
    
#endif 
        
#endif

    setViewport(properties.screen_width, properties.screen_height);

#if defined(__EMSCRIPTEN__)
    update_canvas_size();
#endif

    return 0;
}

// get Time Function
const double getTimeSec() {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    timespec temp;
    if ((now.tv_nsec-time_start.tv_nsec)<0) {
        temp.tv_sec = now.tv_sec-time_start.tv_sec-1;
        temp.tv_nsec = 1000000000+now.tv_nsec-time_start.tv_nsec;
    } else {
        temp.tv_sec = now.tv_sec-time_start.tv_sec;
        temp.tv_nsec = now.tv_nsec-time_start.tv_nsec;
    }
    return double(temp.tv_sec) + double(temp.tv_nsec/1000000000.);
}

const bool isGL() {
 
#if defined(DRIVER_GLFW)
    return !glfwWindowShouldClose(window);

#elif defined(DRIVER_BROADCOM)
    return bHostInited;

#elif defined(DRIVER_GBM)
    return true;

#endif

    return false;
}

void updateGL() {
    // Update time
    // --------------------------------------------------------------------
    double now = getTimeSec();
    float diff = now - elapseTime;
    if (diff < restSec) {
        sleep_ms(int((restSec - diff) * 1000000));
        now = getTimeSec();
    }    

    delta = now - elapseTime;
    elapseTime = now;

    static int frame_count = 0;
    static double lastTime = 0.0;
    frame_count++;
    lastTime += delta;
    if (lastTime >= 1.) {
        FPS = double(frame_count);
        frame_count = 0;
        lastTime -= 1.;
    }

    // EVENTS
    // --------------------------------------------------------------------
#if !defined(DRIVER_GLFW)
    if ( onMouseMove || onMousePress || onMouseDrag || onMouseRelease ) {
        const int XSIGN = 1<<4, YSIGN = 1<<5;
        static int fd = -1;
        if (fd < 0) {
            fd = open(properties.mouse.c_str(),O_RDONLY|O_NONBLOCK);
        }

        if (fd >= 0) {
            // Set values to 0
            mouse.vel = glm::vec2(0.0, 0.0);

            // Extract values from driver
            struct {char buttons, dx, dy; } m;
            while (1) {
                int bytes = read(fd, &m, sizeof m);

                if (bytes < (int)sizeof m)
                    return;
                else if (m.buttons&8) 
                    break; // This bit should always be set

                read(fd, &m, 1); // Try to sync up again
            }

            // Set button value
            int button = m.buttons&3;
            if (button) mouse.button = button;
            else mouse.button = 0;

            // Set deltas
            mouse.vel.x=m.dx;
            mouse.vel.y=m.dy;
            if (m.buttons&XSIGN) mouse.vel.x-=256;
            if (m.buttons&YSIGN) mouse.vel.y-=256;

            // Add movement
            mouse.pos += mouse.vel;

            // Clamp values
            if (mouse.pos.x < 0) mouse.pos.x=0;
            if (mouse.pos.y < 0) mouse.pos.y=0;
            if (mouse.pos.x > viewport.z) mouse.pos.x = viewport.z;
            if (mouse.pos.y > viewport.w) mouse.pos.y = viewport.w;

            // Lunch events
            if (mouse.button == 0 && button != mouse.button) {
                mouse.button = button;
                if (onMousePress)
                    onMousePress(mouse.pos.x, mouse.pos.y, mouse.button);
            }
            else if (mouse.button != 0 && button != mouse.button) {
                mouse.button = button;
                if (onMouseRelease)
                    onMouseRelease(mouse.pos.x, mouse.pos.y, mouse.button);
            }
            else
                mouse.button = button;

            if (mouse.vel.x != 0.0 || mouse.vel.y != 0.0) {
                if (button != 0) {
                    if (onMouseDrag)
                        onMouseDrag(mouse.pos.x, mouse.pos.y, mouse.button);
                }
                else {
                    if (onMouseMove)
                        onMouseMove(mouse.pos.x, mouse.pos.y);
                }
            }
        }
    }
#endif
}

void renderGL(){
// NON GLFW
    // glFinish();

#if defined(DRIVER_GLFW)
    glfwSwapBuffers(window);
    glfwPollEvents();

#else
    eglSwapBuffers(display, surface);
    #if defined(DRIVER_GBM)
    gbmSwapBuffers();
    #endif

#endif
}

void closeGL(){
// NON GLFW
#if defined(DRIVER_GLFW)
    glfwSetWindowShouldClose(window, GL_TRUE);
    glfwTerminate();

#else
    eglSwapBuffers(display, surface);

    // Release OpenGL resources
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, surface);
    eglDestroyContext(display, context);
    eglTerminate(display);
    eglReleaseThread();

    #if defined(DRIVER_BROADCOM)
    vc_dispmanx_display_close(dispman_display);
    bcm_host_deinit();

    #elif defined(DRIVER_GBM)
    gbmClean();
    close(device);
    #endif

#endif
}


//-------------------------------------------------------------
void setWindowSize(int _width, int _height) {
#if defined(__EMSCRIPTEN__)
    setViewport((float)_width, (float)_height);
    glfwSetWindowSize(window, _width * getPixelDensity(true), _height * getPixelDensity(true));
    return;

#elif defined(DRIVER_GLFW)
    if (properties.style != EMBEDDED)
        glfwSetWindowSize(window, _width / getPixelDensity(true), _height / getPixelDensity(true));
#endif

    setViewport((float)_width / getPixelDensity(true), (float)_height / getPixelDensity(true));
}

void setWindowTitle( const char* _title) {
#if defined(DRIVER_GLFW)
    glfwSetWindowTitle(window, _title);
#endif
}

void setWindowVSync(bool _value) {
#if defined(DRIVER_GLFW)
    glfwSwapInterval(_value);
#endif
}

void setViewport(float _width, float _height) {
    viewport.z = _width;
    viewport.w = _height;

    updateViewport();
}

void setViewport(int _x, int _y, int _width, int _height){
    viewport.x = _x;
    viewport.y = _y;
    viewport.z = _width;
    viewport.w = _height;

    updateViewport();
}

void updateViewport() {
    fPixelDensity = getPixelDensity(true);
    float width = getWindowWidth();
    float height = getWindowHeight();

    if (properties.style != EMBEDDED) {
        glViewport( (float)viewport.x * fPixelDensity, (float)viewport.y * fPixelDensity,
                    width, height);
    }

    orthoMatrix = glm::ortho(   (float)viewport.x * fPixelDensity, width, 
                                (float)viewport.y * fPixelDensity, height);

    orthoFlippedMatrix = glm::ortho(   (float)viewport.x * fPixelDensity, width, 
                                        height, (float)viewport.y * fPixelDensity);

    if (onViewportResize)
        onViewportResize(width, height);
}

void getScreenSize() {
#if defined(DRIVER_GLFW)
    // glfwGetMonitorPhysicalSize(glfwGetPrimaryMonitor(), &screen.x, &screen.y);
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    screen_width = mode->width;
    screen_height = mode->height;

#elif defined(DRIVER_BROADCOM)
    initHost();
    int32_t success = graphics_get_display_size(0 /* LCD */, &screen_width, &screen_height);
    assert(success >= 0);

#elif defined(DRIVER_GBM)
    initHost();
    screen_width = mode.hdisplay;
    screen_height = mode.vdisplay;

#endif
}

const int getScreenWidth() {
    if (screen_width <= 0.0)
        getScreenSize();
    return screen_width;
}

const int getScreenHeight() {
    if (screen_height <= 0.0)
        getScreenSize();
    return screen_height;
}

const bool isFullscreen() {
#if defined(__EMSCRIPTEN__)
    return properties.style == FULLSCREEN;
#elif defined(DRIVER_GLFW)
    return glfwGetWindowMonitor( window ) != nullptr;
#else 
    return properties.style == FULLSCREEN;
#endif
}

void setFullscreen(bool _fullscreen) {
    if ( isFullscreen() == _fullscreen || 
         properties.style == EMBEDDED)
        return;

#if defined(__EMSCRIPTEN__)
    if ( _fullscreen ) {
        properties.style = FULLSCREEN;

        EmscriptenFullscreenStrategy strategy{};

        strategy.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH;    
        strategy.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT;
        strategy.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_STDDEF;
        strategy.canvasResizedCallback = on_canvassize_changed;
        strategy.canvasResizedCallbackUserData = NULL;
        emscripten_enter_soft_fullscreen("#canvas", &strategy);
    }
    else {
        properties.style = FULLSCREEN;
    }
    
#elif defined(DRIVER_GLFW)
    if ( _fullscreen ) {
        // backup window position and window size
        glfwGetWindowPos( window, &viewport_last.x, &viewport_last.y );
        glfwGetWindowSize( window, &viewport_last.z, &viewport_last.w );
        
        // get resolution of monitor
        GLFWmonitor* monitor = getMonitorFromWindow(window);
        const GLFWvidmode * mode = glfwGetVideoMode(monitor);

        // switch to full screen
        glfwSetWindowMonitor( window, monitor, 0, 0, mode->width, mode->height, 0 );
        properties.style = FULLSCREEN;
    }
    else {
        // restore last window size and position
        glfwSetWindowMonitor( window, nullptr,  viewport_last.x, viewport_last.y, viewport_last.z, viewport_last.w, 0 );
        properties.style = REGULAR;
    }

#endif
}

void setPixelDensity(float _density) { fPixelDensity = std::max(1.0f,_density); }
const float getPixelDensity(bool _compute) {
    if (_compute && properties.style != EMBEDDED) {
#if defined(__EMSCRIPTEN__)
        return std::max(1.0f, float(emscripten_get_device_pixel_ratio()));
#elif defined(DRIVER_GLFW)
        int window_width, window_height, framebuffer_width, framebuffer_height;
        glfwGetWindowSize(window, &window_width, &window_height);
        glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
        return std::max(1.0f, float(framebuffer_width))/std::max(1.0f, float(window_width));
#else
        return std::max(1.0f, fPixelDensity);
#endif
    }
    else
        return std::max(1.0f, fPixelDensity);
}

void setWindowIcon(unsigned char* _data, size_t _width, size_t _height) {
#if defined(DRIVER_GLFW)
    GLFWimage image;
    image.pixels = _data;
    image.width = _width;
    image.height = _height;
    glfwSetWindowIcon(window, 1, &image);
#endif
}

const glm::ivec4& getViewport() { 
    // int vp[4];
    // glGetIntegerv(GL_VIEWPORT, vp);
    // return glm::ivec4(vp[0], vp[1], vp[2], vp[3]);
    return viewport;
}

const glm::mat4& getOrthoMatrix() { return orthoMatrix; }
const glm::mat4& getFlippedOrthoMatrix() { return orthoFlippedMatrix; }
const int getWindowWidth() { return viewport.z * fPixelDensity; }
const int getWindowHeight() { return viewport.w * fPixelDensity; }
const int getWindowMSAA() { return properties.msaa; }
const WindowStyle getWindowStyle() { return properties.style; }

const std::string& getVendor() {
    if (properties.vendor == "") properties.vendor = std::string((const char*)glGetString(GL_VENDOR));
    return properties.vendor;
}

const std::string& getRenderer() {
    if (properties.renderer == "") properties.renderer = std::string((const char*)glGetString(GL_RENDERER));
    return properties.renderer;
}

const std::string& getGLVersion() {
    if (properties.version == "") properties.version = std::string((const char*)glGetString(GL_VERSION));
    return properties.version;
}

const std::string& getGLSLVersion() {
    if (properties.glsl == "") properties.glsl = std::string((const char *)glGetString(GL_SHADING_LANGUAGE_VERSION));
    return properties.glsl;
}

const std::string& getExtensions() { 
    if (properties.extensions == "") properties.extensions = std::string((const char*)glGetString(GL_EXTENSIONS));
    return properties.extensions; 
}

const bool haveExtension(const std::string& _name) { 
    std::string gl_version = getGLVersion();
    if (gl_version[0] == '4')
        return true;

    if (properties.extensions == "") properties.extensions = std::string((const char*)glGetString(GL_EXTENSIONS));
    return properties.extensions.find(_name) == std::string::npos; 
}

#if defined(__EMSCRIPTEN__)
const size_t getWebGLVersionNumber() {
    if (properties.webgl == 0) properties.webgl = (beginsWith( getGLVersion(), "OpenGL ES 2.0"))? 1 : 2 ;
    return properties.webgl;
}
#endif

glm::vec4 getDate() {
#if defined(_MSC_VER)
    time_t tv = time(NULL);

    struct tm tm_struct;
    struct tm* tm = &tm_struct;
    errno_t err = localtime_s(tm, &tv);
    if (err) {
            
    }

    return glm::vec4(tm->tm_year + 1900,
        tm->tm_mon,
        tm->tm_mday,
        tm->tm_hour * 3600.0f + tm->tm_min * 60.0f + tm->tm_sec);
#else
    gettimeofday(&tv, NULL);
    struct tm *tm;
    tm = localtime(&tv.tv_sec);
    // std::cout << "y: " << tm->tm_year+1900 << " m: " << tm->tm_mon << " d: " << tm->tm_mday << " s: " << tm->tm_hour*3600.0f+tm->tm_min*60.0f+tm->tm_sec+tv.tv_usec*0.000001 << std::endl;
    return glm::vec4(tm->tm_year + 1900,
        tm->tm_mon,
        tm->tm_mday,
        tm->tm_hour * 3600.0f + tm->tm_min * 60.0f + tm->tm_sec + tv.tv_usec * 0.000001);
#endif 
}

const double  getTime() { return elapseTime;}
const double  getDelta() { return delta; }

void    setFps(int _fps) { 
    if (_fps == 0)
        restSec = 0.0;
    else
        restSec = 1.0f/(float)_fps; 
}
const double  getFps() { return 1.0/restSec; }

const float   getRestSec() { return restSec; }
const int     getRestMs() { return restSec * 1000; }
const int     getRestUs() { return restSec * 1000000; }

void    setMousePosition( float _x, float _y ) {
    mouse.pos = glm::vec2(_x, _y);
    mouse.vel = glm::vec2(0.0f, 0.0f);
    mouse.drag = glm::vec2(_x, _y);
    #if defined(DRIVER_GLFW)
    float h = getWindowHeight();
    float y = _y;//h - glm::clamp(_y, 0.0f, h);
    glfwSetCursorPos(window, _x / fPixelDensity , y / fPixelDensity);
    #endif
}

void    setMouseVisibility(bool _visible) {
    #if defined(DRIVER_GLFW)
    if (_visible)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    else
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    
    #endif
}

const glm::vec2& getMousePosition() { return mouse.pos; }
const glm::vec2  getMousePositionFlipped(){ return glm::vec2(mouse.pos.x, getWindowHeight() - mouse.pos.y); }
const float   getMouseX(){ return mouse.pos.x; }
const float   getMouseY(){ return mouse.pos.y; }
const float   getMouseYFlipped(){ return getWindowHeight() - mouse.pos.y; }

const glm::vec2& getMouseVelocity() { return mouse.vel; }
const glm::vec2  getMouseVelocityFlipped(){ return glm::vec2(mouse.vel.x, -mouse.vel.y); }
const float   getMouseVelX(){ return mouse.vel.x; }
const float   getMouseVelY(){ return mouse.vel.y;}
const float   getMouseVelXFlipped(){ return -mouse.vel.y; }

const int     getMouseButton(){ return mouse.button;}
const bool    getMouseEntered() { return mouse.entered; }

const bool    isShiftPressed() { return bShift; }
const bool    isControlPressed() { return bControl; }

}
