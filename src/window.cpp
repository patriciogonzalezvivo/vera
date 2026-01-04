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
static glm::mat4                ortho_matrix;
static glm::mat4                ortho_flipped_matrix;

typedef struct {
    glm::vec2   pos;
    glm::vec2   vel;
    glm::vec2   drag;
    int         button;
    bool        entered;
} Mouse;
static Mouse                    mouse;

struct timeval                  tv;
static uint32_t                 screen_width        = 0;
static uint32_t                 screen_height       = 0;
static double                   elapsed_time        = 0.0;
static double                   delta               = 0.0;
static double                   FPS                 = 0.0;
static double                   rest_sec             = 0.0167; // default 60fps 
static float                    device_pixel_ratio  = 1.0f;

static bool                     bShift          = false;    
static bool                     bControl        = false;    

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
    static bool                     left_mouse_button_down = false;
    static GLFWwindow*              window;

    #if defined(PLATFORM_RPI)
    #include "GLFW/glfw3native.h"
    EGLDisplay getEGLDisplay() { return glfwGetEGLDisplay(); }
    EGLContext getEGLContext() { return glfwGetEGLContext(window); }
    #endif

    typedef struct _rect { int x, y; int w, h; } _rect;
    int _min(int a, int b) { return a < b ? a : b; }
    int _max(int a, int b) { return a > b ? a : b; }
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

#else // NON-GLFW (DIRECT MODE)

    #include <assert.h>
    #include <fcntl.h>
    #include <termios.h>
    #include <stdio.h> 
    #include <unistd.h>

    #if defined(DRIVER_BROADCOM)
        #include "bcm_host.h"
        #undef countof

        // EGL / GLES2
        #include <EGL/egl.h>
        #include <EGL/eglext.h>
        #include <GLES2/gl2.h>
        #include <GLES2/gl2ext.h>

        DISPMANX_DISPLAY_HANDLE_T dispman_display;

    #elif defined(DRIVER_DRM)
        #include <errno.h>

        #include <xf86drm.h>
        #include <xf86drmMode.h>
        
        #include <gbm.h>
        #include <drm_fourcc.h>

        // DRM
        #define MAX_DRM_DEVICES 64
        #ifndef DRM_FORMAT_MOD_LINEAR
        #define DRM_FORMAT_MOD_LINEAR 0
        #endif
        #ifndef DRM_FORMAT_MOD_INVALID
        #define DRM_FORMAT_MOD_INVALID ((((__u64)0) << 56) | ((1ULL << 56) - 1))
        #endif

        int                     drm_device;
        drmModeModeInfo*        drm_mode;
        drmModeCrtc*            drm_crtc;
        uint32_t                drm_connector_id;
        uint32_t                drm_crtc_id;
        uint32_t                drm_crtc_index;

        uint32_t                drm_format = DRM_FORMAT_XRGB8888;
        uint64_t                modifier = DRM_FORMAT_MOD_LINEAR;
        uint32_t                drm_vrefresh;
        bool                    drm_async_page_flip = false;
        uint32_t                drm_flags = DRM_MODE_PAGE_FLIP_EVENT;

        static void page_flip_handler(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data) {
            /* suppress 'unused parameter' warnings */
            (void)fd, (void)frame, (void)sec, (void)usec;
            int *waiting_for_flip = (int*)data;
            *waiting_for_flip = 0;
        }

        fd_set                  drm_fds;
        drmEventContext         drm_evctx = {
                .version            = 2,
                .page_flip_handler  = page_flip_handler,
        };

        static int get_resources(int fd, drmModeRes **resources) {
            *resources = drmModeGetResources(fd);
            if (*resources == NULL)
                return EXIT_FAILURE;
            return 0;
        }

        static int find_drm_device(drmModeRes **resources) {
            drmDevicePtr devices[MAX_DRM_DEVICES] = { NULL };
            int num_devices, fd = -1;

            num_devices = drmGetDevices2(0, devices, MAX_DRM_DEVICES);
            printf("Number of devices %d\n", num_devices);
            if (num_devices < 0) {
                printf("drmGetDevices2 failed: %s\n", strerror(-num_devices));
                return -1;
            }

            for (int i = 0; i < num_devices; i++) {
                drmDevicePtr device = devices[i];
                int ret;

                if (!(device->available_nodes & (1 << DRM_NODE_PRIMARY)))
                    continue;
                /* OK, it's a primary device. If we can get the
                * drmModeResources, it means it's also a
                * KMS-capable device.
                */
                fd = open(device->nodes[DRM_NODE_PRIMARY], O_RDWR);
                if (fd < 0)
                    continue;
                ret = get_resources(fd, resources);
                if (!ret)
                    break;
                close(fd);
                fd = -1;
            }
            drmFreeDevices(devices, num_devices);

            if (fd < 0)
                printf("no drm device found!\n");
            return fd;
        }

        static int32_t find_crtc_for_encoder(const drmModeRes *resources, const drmModeEncoder *encoder) {
            for (int i = 0; i < resources->count_crtcs; i++) {
                /* possible_crtcs is a bitmask as described here:
                * https://dvdhrm.wordpress.com/2012/09/13/linux-drm-mode-setting-api
                */
                const uint32_t crtc_mask = 1 << i;
                const uint32_t crtc_id = resources->crtcs[i];
                if (encoder->possible_crtcs & crtc_mask) {
                    return crtc_id;
                }
            }

            /* no match found */
            return EXIT_FAILURE;
        }


        static int32_t find_crtc_for_connector(const drmModeRes *resources, const drmModeConnector *connector) {
            for (int i = 0; i < connector->count_encoders; i++) {
                const uint32_t encoder_id = connector->encoders[i];
                drmModeEncoder *encoder = drmModeGetEncoder(drm_device, encoder_id);

                if (encoder) {
                    const int32_t crtc_id = find_crtc_for_encoder(resources, encoder);

                    drmModeFreeEncoder(encoder);
                    if (crtc_id != 0) {
                        return crtc_id;
                    }
                }
            }

            /* no match found */
            return EXIT_FAILURE;
        }

        int init_drm(const char *device, const char *mode_str, unsigned int vrefresh) {
            drmModeRes *resources;
            drmModeConnector *connector = NULL;
            drmModeEncoder *encoder = NULL;
            int i, ret, area;

            if (vera::urlExists(std::string(device))) {
                drm_device = open(device, O_RDWR);
                ret = get_resources(drm_device, &resources);
                if (ret < 0 && errno == EOPNOTSUPP)
                    printf("%s does not look like a modeset device\n", device);
            } else {
                drm_device = find_drm_device(&resources);
            }
            

            if (drm_device < 0) {
                printf("could not open drm device\n");
                return -1;
            }

            if (!resources) {
                printf("drmModeGetResources failed: %s\n", strerror(errno));
                return -1;
            }

            /* find a connected connector: */
            for (i = 0; i < resources->count_connectors; i++) {
                connector = drmModeGetConnector(drm_device, resources->connectors[i]);
                if (connector->connection == DRM_MODE_CONNECTED) {
                    /* it's connected, let's use this! */
                    break;
                }
                drmModeFreeConnector(connector);
                connector = NULL;
            }

            if (!connector) {
                /* we could be fancy and listen for hotplug events and wait for
                * a connector..
                */
                printf("no connected connector!\n");
                return -1;
            }

            /* find user requested mode: */
            if (mode_str && *mode_str) {
                for (i = 0; i < connector->count_modes; i++) {
                    drmModeModeInfo *current_mode = &connector->modes[i];

                    if (strcmp(current_mode->name, mode_str) == 0) {
                        if (vrefresh == 0 || current_mode->vrefresh == vrefresh) {
                            drm_mode = current_mode;
                            break;
                        }
                    }
                }

                if (!drm_mode)
                    printf("requested mode not found, using default mode!\n");
            }

            /* find preferred mode or the highest resolution mode: */
            if (!drm_mode) {
                for (i = 0, area = 0; i < connector->count_modes; i++) {
                    drmModeModeInfo *current_mode = &connector->modes[i];

                    if (current_mode->type & DRM_MODE_TYPE_PREFERRED) {
                        drm_mode = current_mode;
                        break;
                    }

                    std::cout << "current_mode: " << current_mode->hdisplay << "x" << current_mode->vdisplay << std::endl;

                    int current_area = current_mode->hdisplay * current_mode->vdisplay;
                    if (current_area > area) {
                        drm_mode = current_mode;
                        area = current_area;
                    }
                }
            }

            if (!drm_mode) {
                printf("could not find mode!\n");
                return -1;
            }

            /* find encoder: */
            for (i = 0; i < resources->count_encoders; i++) {
                encoder = drmModeGetEncoder(drm_device, resources->encoders[i]);
                if (encoder->encoder_id == connector->encoder_id)
                    break;
                drmModeFreeEncoder(encoder);
                encoder = NULL;
            }

            if (encoder) {
                drm_crtc_id = encoder->crtc_id;
            } 
            else {
                uint32_t crtc_id = find_crtc_for_connector(resources, connector);
                if (crtc_id == 0) {
                    printf("no crtc found!\n");
                    return -1;
                }
                drm_crtc_id = crtc_id;
            }

            for (i = 0; i < resources->count_crtcs; i++) {
                if (resources->crtcs[i] == drm_crtc_id) {
                    drm_crtc_index = i;
                    break;
                }
            }

            drmModeFreeResources(resources);

            drm_connector_id = connector->connector_id;

            return 0;
        }

        // GBM
        //
        #define WEAK __attribute__((weak))
        WEAK struct gbm_surface * gbm_surface_create_with_modifiers(struct gbm_device *gbm, uint32_t width, uint32_t height, uint32_t format, const uint64_t *modifiers, const unsigned int count);
        WEAK union gbm_bo_handle gbm_bo_get_handle_for_plane(struct gbm_bo *bo, int plane);

        struct gbm {
            struct gbm_device*  device;
            struct gbm_surface* surface;

            uint32_t            format;
            int                 width, height;
        };

        struct gbm_fb {
            struct gbm_bo*      bo;
            uint32_t            fb_id;
        };

        enum mode {
            SMOOTH,        /* smooth-shaded */
            RGBA,          /* single-plane RGBA */
            NV12_2IMG,     /* NV12, handled as two textures and converted to RGB in shader */
            NV12_1IMG,     /* NV12, imported as planar YUV eglimg */
            VIDEO,         /* video textured cube */
        };

        static struct gbm       gbm;
        static struct gbm_bo*   gbm_curr_bo = NULL;
        static uint32_t         gbm_curr_fb;

        static void gbm_fb_destroy_callback(struct gbm_bo* bo, void* data) {
            int drm_fd = gbm_device_get_fd(gbm_bo_get_device(bo));
            struct gbm_fb* fb = data;

            if (fb->fb_id)
                drmModeRmFB(drm_fd, fb->fb_id);

            free(fb);
        }

        struct gbm_fb* gbm_fb_get_from_bo(struct gbm_bo *bo) {
            int drm_fd = gbm_device_get_fd(gbm_bo_get_device(bo));
            struct gbm_fb* fb = gbm_bo_get_user_data(bo);
            uint32_t width, height, format,
                strides[4] = {0}, handles[4] = {0},
                offsets[4] = {0}, flags = 0;
            int ret = -1;

            if (fb)
                return fb;

            fb = (gbm_fb*)calloc(1, sizeof *fb);
            fb->bo = bo;

            width = gbm_bo_get_width(bo);
            height = gbm_bo_get_height(bo);
            format = gbm_bo_get_format(bo);

            if (gbm_bo_get_handle_for_plane && gbm_bo_get_modifier &&
                gbm_bo_get_plane_count && gbm_bo_get_stride_for_plane &&
                gbm_bo_get_offset) {

                uint64_t modifiers[4] = {0};
                modifiers[0] = gbm_bo_get_modifier(bo);
                const int num_planes = gbm_bo_get_plane_count(bo);
                for (int i = 0; i < num_planes; i++) {
                    handles[i] = gbm_bo_get_handle_for_plane(bo, i).u32;
                    strides[i] = gbm_bo_get_stride_for_plane(bo, i);
                    offsets[i] = gbm_bo_get_offset(bo, i);
                    modifiers[i] = modifiers[0];
                }

                if (modifiers[0] && modifiers[0] != DRM_FORMAT_MOD_INVALID) {
                    flags = DRM_MODE_FB_MODIFIERS;
                }

                ret = drmModeAddFB2WithModifiers(drm_fd, width, height,
                        format, handles, strides, offsets,
                        modifiers, &fb->fb_id, flags);
            }

            if (ret) {
                if (flags)
                    fprintf(stderr, "Modifiers failed!\n");

                uint32_t handles_arr[] = {gbm_bo_get_handle(bo).u32,0,0,0};
                uint32_t strides_arr[] = {gbm_bo_get_stride(bo),0,0,0};
                memcpy(handles, handles_arr, 16);
                memcpy(strides, strides_arr, 16);
                memset(offsets, 0, 16);
                ret = drmModeAddFB2(drm_fd, width, height, format, handles, strides, offsets, &fb->fb_id, 0);
            }

            if (ret) {
                printf("failed to create fb: %s\n", strerror(errno));
                free(fb);
                return NULL;
            }

            gbm_bo_set_user_data(bo, fb, gbm_fb_destroy_callback);

            return fb;
        }

        int gbm_init(int drm_fd, int w, int h, uint32_t format, uint64_t modifier) {
            // static struct gbm gbm;

            gbm.device = gbm_create_device(drm_fd);
            gbm.surface = NULL;

            if (!gbm.device)
                return EXIT_FAILURE;

            if (gbm_surface_create_with_modifiers)
                gbm.surface = gbm_surface_create_with_modifiers(gbm.device, w, h, format, &modifier, 1);

            if (!gbm.surface) {
                if (modifier != DRM_FORMAT_MOD_LINEAR) {
                    fprintf(stderr, "Modifiers requested but support isn't available\n");
                    return EXIT_FAILURE;
                }

                gbm.surface = gbm_surface_create(gbm.device, w, h, format, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
            }

            if (!gbm.surface) {
                printf("failed to create GBM surface\n");
                return EXIT_FAILURE;
            }
            else {
                printf("GBM surface created at %dx%d\n", w, h);
                printf("===================================\n");
            }

            return 0;
        }        

        static void gbm_clean() {
            // set the previous crtc
            drmModeSetCrtc(drm_device, drm_crtc->crtc_id, drm_crtc->buffer_id, drm_crtc->x, drm_crtc->y, &drm_connector_id, 1, &drm_crtc->mode);
            drmModeFreeCrtc(drm_crtc);
            if (gbm_curr_bo) {
                drmModeRmFB(drm_device, gbm_curr_fb);
                gbm_surface_release_buffer(gbm.surface, gbm_curr_bo);
            }
            gbm_surface_destroy(gbm.surface);
            gbm_device_destroy(gbm.device);
        }

        static void gbm_swap_buffers() {
            int waiting_for_flip = 1;
            struct gbm_bo* next_bo = gbm_surface_lock_front_buffer(gbm.surface);
            struct gbm_fb* fb = gbm_fb_get_from_bo(next_bo);
            if (!fb) {
                fprintf(stderr, "Failed to get a new framebuffer BO\n");
                return;
            }

            /*
            * Here you could also update drm plane layers if you want
            * hw composition
            */
            
            int ret = drmModePageFlip(drm_device, drm_crtc_id, fb->fb_id, drm_flags, &waiting_for_flip);
            if (ret) {
                printf("failed to queue page flip: %s\n", strerror(errno));
                return;
            }

            while (waiting_for_flip) {
                FD_ZERO(&drm_fds);
                FD_SET(0, &drm_fds);
                FD_SET(drm_device, &drm_fds);
            
                ret = select(drm_device + 1, &drm_fds, NULL, NULL, NULL);
                if (ret < 0) {
                    printf("select err: %s\n", strerror(errno));
                    return;
                } else if (ret == 0) {
                    printf("select timeout!\n");
                    return;
                } else if (FD_ISSET(0, &drm_fds)) {
                    printf("user interrupted!\n");
                    return;
                }
                drmHandleEvent(drm_device, &drm_evctx);
            }
            
            /* release last buffer to render on again: */
            if (gbm.surface) {
                gbm_surface_release_buffer(gbm.surface, next_bo);
            }
            gbm_curr_bo = next_bo;
        }


        int drm_host_init() {
            if (init_drm(properties.display.c_str(), properties.mode, drm_vrefresh) == -1) {
                printf("drm init failed\n");
                return -1;
            }

            screen_width = drm_mode->hdisplay;
            screen_height = drm_mode->vdisplay;
            return gbm_init(drm_device, screen_width, screen_height, drm_format, modifier);
        }

        #endif /* DRIVER_DRM */

        // EGL / GLES2
        #include <EGL/egl.h>
        #include <EGL/eglext.h>
        #include <GLES2/gl2.h>
        #include <GLES2/gl2ext.h>

        #ifndef EGL_KHR_platform_gbm
        #define EGL_KHR_platform_gbm 1
        #define EGL_PLATFORM_GBM_KHR              0x31D7
        #endif /* EGL_KHR_platform_gbm */

        #ifndef EGL_EXT_platform_base
        #define EGL_EXT_platform_base 1
        typedef EGLDisplay (EGLAPIENTRYP PFNEGLGETPLATFORMDISPLAYEXTPROC) (EGLenum platform, void *native_display, const EGLint *attrib_list);
        typedef EGLSurface (EGLAPIENTRYP PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC) (EGLDisplay dpy, EGLConfig config, void *native_window, const EGLint *attrib_list);
        typedef EGLSurface (EGLAPIENTRYP PFNEGLCREATEPLATFORMPIXMAPSURFACEEXTPROC) (EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLint *attrib_list);
        #ifdef EGL_EGLEXT_PROTOTYPES
        EGLAPI EGLDisplay EGLAPIENTRY eglGetPlatformDisplayEXT (EGLenum platform, void *native_display, const EGLint *attrib_list);
        EGLAPI EGLSurface EGLAPIENTRY eglCreatePlatformWindowSurfaceEXT (EGLDisplay dpy, EGLConfig config, void *native_window, const EGLint *attrib_list);
        EGLAPI EGLSurface EGLAPIENTRY eglCreatePlatformPixmapSurfaceEXT (EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLint *attrib_list);
        #endif
        #endif /* EGL_EXT_platform_base */

        #ifndef EGL_VERSION_1_5
        #define EGLImage EGLImageKHR
        #endif /* EGL_VERSION_1_5 */

        #define WEAK __attribute__((weak))

        /* Define tokens from EGL_EXT_image_dma_buf_import_modifiers */
        #ifndef EGL_EXT_image_dma_buf_import_modifiers
        #define EGL_EXT_image_dma_buf_import_modifiers 1
        #define EGL_DMA_BUF_PLANE3_FD_EXT         0x3440
        #define EGL_DMA_BUF_PLANE3_OFFSET_EXT     0x3441
        #define EGL_DMA_BUF_PLANE3_PITCH_EXT      0x3442
        #define EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT 0x3443
        #define EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT 0x3444
        #define EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT 0x3445
        #define EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT 0x3446
        #define EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT 0x3447
        #define EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT 0x3448
        #define EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT 0x3449
        #define EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT 0x344A
        #endif

        struct egl {
            EGLDisplay  display;
            EGLContext  context;
            EGLSurface  surface;

            EGLConfig   config;
            
            #if defined(DRIVER_DRM)
            PFNEGLGETPLATFORMDISPLAYEXTPROC     eglGetPlatformDisplayEXT;
            PFNEGLCREATEIMAGEKHRPROC            eglCreateImageKHR;
            PFNEGLDESTROYIMAGEKHRPROC           eglDestroyImageKHR;
            PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;
            PFNEGLCREATESYNCKHRPROC             eglCreateSyncKHR;
            PFNEGLDESTROYSYNCKHRPROC            eglDestroySyncKHR;
            PFNEGLWAITSYNCKHRPROC               eglWaitSyncKHR;
            PFNEGLCLIENTWAITSYNCKHRPROC         eglClientWaitSyncKHR;
            PFNEGLDUPNATIVEFENCEFDANDROIDPROC   eglDupNativeFenceFDANDROID;

            /* AMD_performance_monitor */
            PFNGLGETPERFMONITORGROUPSAMDPROC         glGetPerfMonitorGroupsAMD;
            PFNGLGETPERFMONITORCOUNTERSAMDPROC       glGetPerfMonitorCountersAMD;
            PFNGLGETPERFMONITORGROUPSTRINGAMDPROC    glGetPerfMonitorGroupStringAMD;
            PFNGLGETPERFMONITORCOUNTERSTRINGAMDPROC  glGetPerfMonitorCounterStringAMD;
            PFNGLGETPERFMONITORCOUNTERINFOAMDPROC    glGetPerfMonitorCounterInfoAMD;
            PFNGLGENPERFMONITORSAMDPROC              glGenPerfMonitorsAMD;
            PFNGLDELETEPERFMONITORSAMDPROC           glDeletePerfMonitorsAMD;
            PFNGLSELECTPERFMONITORCOUNTERSAMDPROC    glSelectPerfMonitorCountersAMD;
            PFNGLBEGINPERFMONITORAMDPROC             glBeginPerfMonitorAMD;
            PFNGLENDPERFMONITORAMDPROC               glEndPerfMonitorAMD;
            PFNGLGETPERFMONITORCOUNTERDATAAMDPROC    glGetPerfMonitorCounterDataAMD;
            #endif /* DRIVER_DRM */

            bool modifiers_supported;

            // void (*draw)(uint64_t start_time, unsigned frame);
        };
        static struct egl egl;

        #define egl_check() assert(glGetError() == 0)

        // Get the EGL error back as a string. Useful for debugging.
        static const char *egl_get_error_str() {
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

        static bool egl_has_ext(const char *extension_list, const char *ext) {
            const char *ptr = extension_list;
            int len = strlen(ext);

            if (ptr == NULL || *ptr == '\0')
                return false;

            while (true) {
                ptr = strstr(ptr, ext);
                if (!ptr)
                    return false;

                if (ptr[len] == ' ' || ptr[len] == '\0')
                    return true;

                ptr += len;
            }
        }

        static int match_config_to_visual(EGLint visual_id, EGLConfig *configs, int count) {
            int i;

            for (i = 0; i < count; ++i) {
                EGLint id;

                if (!eglGetConfigAttrib(egl.display, configs[i], EGL_NATIVE_VISUAL_ID, &id))
                    continue;

                if (id == visual_id)
                    return i;
            }

            return EXIT_FAILURE;
        }

        static bool egl_choose_config(const EGLint *attribs, EGLint visual_id, EGLConfig *config_out) {
            EGLint count = 0;
            EGLint matched = 0;
            EGLConfig* configs;
            int config_index = -1;

            if (!eglGetConfigs(egl.display, NULL, 0, &count) || count < 1) {
                printf("No EGL configs to choose from.\n");
                return false;
            }
            configs = (EGLConfig*)malloc(count * sizeof *configs);
            if (configs == NULL) {
                printf("Failed to allocate memory for EGL configs.\n");
                return false;
            }
    
            if (!eglChooseConfig(egl.display, attribs, configs, count, &matched) || !matched) {
                printf("No EGL configs with appropriate attributes.\n");
                goto out;
            }

            if (!visual_id)
                config_index = 0;

            if (config_index == -1)
                config_index = match_config_to_visual(visual_id, configs, matched);

            if (config_index != -1)
                *config_out = configs[config_index];

        out:
            free(configs);
            if (config_index == -1)
                return false;

            return true;
        }
        
    namespace vera {

        EGLDisplay getEGLDisplay() { return egl.display; }
        EGLContext getEGLContext() { return egl.context; }
    
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

#if !defined(DRIVER_GLFW)
    static bool bHostInited = false;
    static void init_host() {
        if (bHostInited)
            return;

        #if defined(DRIVER_BROADCOM)
            bcm_host_init();

        #elif defined(DRIVER_DRM)
            drm_host_init();
        #endif

        bHostInited = true;
    }

    static EGLDisplay getDisplay() {
        init_host();
        // printf("resolution: %ix%i\n", mode.hdisplay, mode.vdisplay);

        #if defined(DRIVER_BROADCOM)
            return eglGetDisplay(EGL_DEFAULT_DISPLAY);

        #elif defined(DRIVER_DRM)

            return eglGetDisplay(gbm.device);
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

WindowProperties getWindowProperties() { return properties; }

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
        properties.screen_width = getDisplayWidth();

    if (properties.screen_height == -1 || properties.style == FULLSCREEN)
        properties.screen_height = getDisplayHeight();

    // Clear application state
    EGLint major, minor;
    EGLBoolean result;

    egl.display = getDisplay();
    assert(egl.display != EGL_NO_DISPLAY);
    egl_check();

    result = eglInitialize(egl.display, NULL, NULL);
    if (EGL_FALSE == result)
        std::cout << "Fail to initialize EGL context." << std::endl;
    assert(EGL_FALSE != result);
    egl_check();

    // Make sure that we can use OpenGL in this EGL app.
    // result = eglBindAPI(EGL_OPENGL_API);
    result = eglBindAPI(EGL_OPENGL_ES_API);
    assert(EGL_FALSE != result);
    egl_check();


    static const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    
    static const EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_SAMPLE_BUFFERS, 1, EGL_SAMPLES, 4,
        #if defined(DRIVER_BROADCOM)
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        #elif defined(DRIVER_DRM)
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        #endif
        EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };

    #if defined(DRIVER_DRM)

    const char *egl_exts_client, *egl_exts_dpy, *gl_exts;

    #define get_proc_client(ext, name) do { \
            if (egl_has_ext(egl_exts_client, #ext)) \
                egl.name = (void*)eglGetProcAddress(#name); \
        } while (0)
    #define get_proc_dpy(ext, name) do { \
            if (egl_has_ext(egl_exts_dpy, #ext)) \
                egl.name = (void*)eglGetProcAddress(#name); \
        } while (0)
    #define get_proc_gl(ext, name) do { \
            if (egl_has_ext(gl_exts, #ext)) \
                egl.name = (void*)eglGetProcAddress(#name); \
        } while (0)

    egl_exts_client = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    get_proc_client(EGL_EXT_platform_base, eglGetPlatformDisplayEXT);

    if (egl.eglGetPlatformDisplayEXT) {
        egl.display = egl.eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR, gbm.device, NULL);
    } else {
        egl.display = eglGetDisplay((void*)gbm.device);
    }

    if (!eglInitialize(egl.display, &major, &minor)) {
        printf("failed to initialize\n");
        return EXIT_FAILURE;
    }

    egl_exts_dpy = eglQueryString(egl.display, EGL_EXTENSIONS);
    get_proc_dpy(EGL_KHR_image_base, eglCreateImageKHR);
    get_proc_dpy(EGL_KHR_image_base, eglDestroyImageKHR);
    get_proc_dpy(EGL_KHR_fence_sync, eglCreateSyncKHR);
    get_proc_dpy(EGL_KHR_fence_sync, eglDestroySyncKHR);
    get_proc_dpy(EGL_KHR_fence_sync, eglWaitSyncKHR);
    get_proc_dpy(EGL_KHR_fence_sync, eglClientWaitSyncKHR);
    get_proc_dpy(EGL_ANDROID_native_fence_sync, eglDupNativeFenceFDANDROID);

    egl.modifiers_supported = egl_has_ext(egl_exts_dpy, "EGL_EXT_image_dma_buf_import_modifiers");

    printf("Using display %p with EGL version %d.%d\n", egl.display, major, minor);

    printf("===================================\n");
    printf("EGL information:\n");
    printf("  version: \"%s\"\n", eglQueryString(egl.display, EGL_VERSION));
    printf("  vendor: \"%s\"\n", eglQueryString(egl.display, EGL_VENDOR));
    printf("  client extensions: \"%s\"\n", egl_exts_client);
    printf("  display extensions: \"%s\"\n", egl_exts_dpy);
    printf("===================================\n");

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        printf("failed to bind api EGL_OPENGL_ES_API\n");
        return EXIT_FAILURE;
    }

    if (!egl_choose_config(config_attribs, drm_format, &egl.config)) {
        printf("failed to choose config\n");
        return EXIT_FAILURE;
    }

    #elif defined(DRIVER_BROADCOM)

    // get an appropriate EGL frame buffer configuration
    EGLint matched = 0;
    if (eglChooseConfig(egl.display, config_attribs, &egl.config, 1, &matched) == EGL_FALSE) {
        std::cerr << "Failed to get EGL configs! Error: " << egl_get_error_str() << std::endl;
        eglTerminate(egl.display);
        #if defined(DRIVER_GBM)
        gbmClean();
        #endif
        return EXIT_FAILURE;
    }
    #endif

    egl.context = eglCreateContext(egl.display, egl.config, EGL_NO_CONTEXT, context_attribs);
    if (egl.context == EGL_NO_CONTEXT) {
        printf("failed to create context\n");
        eglTerminate(egl.display);
        #if defined(DRIVER_DRM)
        gbm_clean();
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
        egl_check();

        egl.surface = eglCreateWindowSurface(egl.display, egl.config, &nativeviewport, NULL );
        assert(egl.surface != EGL_NO_SURFACE);
        egl_check();

        // connect the context to the surface
        result = eglMakeCurrent(egl.display, egl.surface, egl.surface, egl.context);
        assert(EGL_FALSE != result);
        egl_check();

    #elif defined(DRIVER_DRM)
        // egl.surface = eglCreateWindowSurface(egl.display, egl.config, gbm.surface, NULL);
        // if (egl.surface == EGL_NO_SURFACE) {
        //     std::cerr << "Failed to create EGL surface! Error: " << egl_get_error_str() << std::endl;
        //     eglDestroyContext(egl.display, egl.context);
        //     eglTerminate(egl.display);
        //     gbm_clean();
        //     return EXIT_FAILURE;
        // }

        if (!gbm.surface) {
            egl.surface = EGL_NO_SURFACE;
        } else {
            egl.surface = eglCreateWindowSurface(egl.display, egl.config, (EGLNativeWindowType)gbm.surface, NULL);
            if (egl.surface == EGL_NO_SURFACE) {
                std::cerr << "Failed to create EGL surface! Error: " << egl_get_error_str() << std::endl;
                eglDestroyContext(egl.display, egl.context);
                eglTerminate(egl.display);
                gbm_clean();
                return EXIT_FAILURE;
            }
        }

        /* connect the context to the surface */
        eglMakeCurrent(egl.display, egl.surface, egl.surface, egl.context);

        gl_exts = (char *) glGetString(GL_EXTENSIONS);
        printf("OpenGL ES:\n");
        printf("  version: \"%s\"\n", glGetString(GL_VERSION));
        printf("  shading language version: \"%s\"\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
        printf("  vendor: \"%s\"\n", glGetString(GL_VENDOR));
        printf("  renderer: \"%s\"\n", glGetString(GL_RENDERER));
        printf("  extensions: \"%s\"\n", gl_exts);
        printf("===================================\n");

        get_proc_gl(GL_OES_EGL_image, glEGLImageTargetTexture2DOES);

        get_proc_gl(GL_AMD_performance_monitor, glGetPerfMonitorGroupsAMD);
        get_proc_gl(GL_AMD_performance_monitor, glGetPerfMonitorCountersAMD);
        get_proc_gl(GL_AMD_performance_monitor, glGetPerfMonitorGroupStringAMD);
        get_proc_gl(GL_AMD_performance_monitor, glGetPerfMonitorCounterStringAMD);
        get_proc_gl(GL_AMD_performance_monitor, glGetPerfMonitorCounterInfoAMD);
        get_proc_gl(GL_AMD_performance_monitor, glGenPerfMonitorsAMD);
        get_proc_gl(GL_AMD_performance_monitor, glDeletePerfMonitorsAMD);
        get_proc_gl(GL_AMD_performance_monitor, glSelectPerfMonitorCountersAMD);
        get_proc_gl(GL_AMD_performance_monitor, glBeginPerfMonitorAMD);
        get_proc_gl(GL_AMD_performance_monitor, glEndPerfMonitorAMD);
        get_proc_gl(GL_AMD_performance_monitor, glGetPerfMonitorCounterDataAMD);

        eglSwapBuffers(egl.display, egl.surface);
        gbm_curr_bo = gbm_surface_lock_front_buffer(gbm.surface);
        struct gbm_fb *fb = gbm_fb_get_from_bo(gbm_curr_bo);
        if (!fb) {
            fprintf(stderr, "Failed to get a new framebuffer BO\n");
            return EXIT_FAILURE;
        }
        int ret = drmModeSetCrtc(drm_device, drm_crtc_id, fb->fb_id, 0, 0, &drm_connector_id, 1, drm_mode);
        if (ret) {
            printf("failed to set mode: %s\n", strerror(errno));
            return ret;
        }

        if (drm_async_page_flip)
            drm_flags = DRM_MODE_PAGE_FLIP_EVENT;
        else
            drm_flags = DRM_MODE_PAGE_FLIP_EVENT;

        properties.screen_width = screen_width;
        properties.screen_height = screen_height;
        glViewport(0, 0, screen_width, screen_height);
    #endif

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
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
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
            properties.screen_x = getDisplayWidth() / 2 - properties.screen_width / 2;

        if (properties.screen_y == -1)
            properties.screen_y = getDisplayHeight() / 2 - properties.screen_height / 2;

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
        if (_action == GLFW_PRESS && (_key == GLFW_KEY_LEFT_SHIFT || _key == GLFW_KEY_RIGHT_SHIFT) )
            bShift = true;
        else if (_action == GLFW_RELEASE && (_key == GLFW_KEY_LEFT_SHIFT || _key == GLFW_KEY_RIGHT_SHIFT) )
            bShift = false;

        if (_action == GLFW_PRESS && (_key == GLFW_KEY_LEFT_CONTROL || _key == GLFW_KEY_RIGHT_CONTROL) )
            bControl = true;
        else if (_action == GLFW_RELEASE && (_key == GLFW_KEY_LEFT_CONTROL || _key == GLFW_KEY_RIGHT_CONTROL) )
            bControl = false;

        if (_key == GLFW_KEY_ESCAPE ||
            _key == GLFW_KEY_RIGHT || _key == GLFW_KEY_LEFT || _key == GLFW_KEY_UP || _key == GLFW_KEY_DOWN) {
            if (onKeyPress)
                onKeyPress(_key);
        }
    });

    glfwSetScrollCallback(window, [](GLFWwindow* _window, double xoffset, double yoffset) {
        if (onScroll)
            onScroll(-yoffset * device_pixel_ratio);
    });

    glfwSetDropCallback(window, [](GLFWwindow* window, int count, const char** paths) {
        if (onDrop)
            onDrop(count, paths);
    });

    glfwSetWindowContentScaleCallback(window, [](GLFWwindow* _window, float xscale, float yscale) {
        device_pixel_ratio = (xscale > yscale ? xscale : yscale);
        std::cout << "Pixel Density: " << device_pixel_ratio << std::endl;

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
        x *= device_pixel_ratio;
        y *= device_pixel_ratio;

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
        int action3 = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_3);
        int button = 0;

        if (action1 == GLFW_PRESS) button = 1;
        else if (action2 == GLFW_PRESS) button = 2;
        else if (action3 == GLFW_PRESS) button = 3;

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
        if (device_pixel_ratio != getDisplayPixelRatio(true))
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

#elif defined(DRIVER_DRM)
    return true;

#endif

    return false;
}

void updateGL() {
    // Update time
    // --------------------------------------------------------------------
    double now = getTimeSec();
    float diff = now - elapsed_time;
    if (diff < rest_sec) {
        sleep_ms(int((rest_sec - diff) * 1000000));
        now = getTimeSec();
    }    

    delta = now - elapsed_time;
    elapsed_time = now;

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
#if defined(DRIVER_GLFW)
    glfwPollEvents();
#else
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
    glFinish();
    eglSwapBuffers(egl.display, egl.surface);
    #if defined(DRIVER_DRM)
    gbm_swap_buffers();
    #endif

#endif
}

void closeGL(){
// NON GLFW
#if defined(DRIVER_GLFW)
    glfwSetWindowShouldClose(window, GL_TRUE);
    glfwTerminate();

#else
    eglSwapBuffers(egl.display, egl.surface);

    // Release OpenGL resources
    eglMakeCurrent(egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(egl.display, egl.surface);
    eglDestroyContext(egl.display, egl.context);
    eglTerminate(egl.display);
    eglReleaseThread();

    #if defined(DRIVER_BROADCOM)
    vc_dispmanx_display_close(dispman_display);
    bcm_host_deinit();

    #elif defined(DRIVER_DRM)
    gbm_clean();
    close(drm_device);
    #endif

#endif
}


//-------------------------------------------------------------
void setWindowSize(int _width, int _height) {
#if defined(__EMSCRIPTEN__)
    setViewport((float)_width, (float)_height);
    glfwSetWindowSize(window, _width * getDisplayPixelRatio(true), _height * getDisplayPixelRatio(true));
    return;

#elif defined(DRIVER_GLFW)
    if (properties.style != EMBEDDED)
        glfwSetWindowSize(window, _width / getDisplayPixelRatio(true), _height / getDisplayPixelRatio(true));
#endif

    setViewport((float)_width / getDisplayPixelRatio(true), (float)_height / getDisplayPixelRatio(true));
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

    if (onViewportResize)
        onViewportResize(_width, _height);
}

void setViewport(int _x, int _y, int _width, int _height){
    viewport.x = _x;
    viewport.y = _y;
    viewport.z = _width;
    viewport.w = _height;

    updateViewport();
}

void setOrthoMatrix(float _left, float _right, float _bottom, float _top) {
    ortho_matrix = glm::ortho(_left , _right, _bottom, _top);
    ortho_flipped_matrix = glm::ortho(_left, _right, _top, _bottom);
}

void updateViewport() {
    device_pixel_ratio = getDisplayPixelRatio(true);
    float width = getWindowWidth();
    float height = getWindowHeight();

    if (properties.style != EMBEDDED) {
        glViewport( (float)viewport.x * device_pixel_ratio, (float)viewport.y * device_pixel_ratio,
                    width, height);
    }

    setOrthoMatrix( (float)viewport.x * device_pixel_ratio, width, 
                    (float)viewport.y * device_pixel_ratio, height );

    // ortho_matrix = glm::ortho(  (float)viewport.x * device_pixel_ratio, width, 
    //                             (float)viewport.y * device_pixel_ratio, height);
    // ortho_flipped_matrix = glm::ortho(  (float)viewport.x * device_pixel_ratio, width, 
    //                                     height, (float)viewport.y * device_pixel_ratio);
}

void getScreenSize() {
#if defined(DRIVER_GLFW)
    // glfwGetMonitorPhysicalSize(glfwGetPrimaryMonitor(), &screen.x, &screen.y);
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    screen_width = mode->width;
    screen_height = mode->height;

#elif defined(DRIVER_BROADCOM)
    init_host();
    int32_t success = graphics_get_display_size(0 /* LCD */, &screen_width, &screen_height);
    assert(success >= 0);

#elif defined(DRIVER_DRM)
    init_host();
#endif
}

const int getDisplayWidth() {
    if (screen_width <= 0.0)
        getScreenSize();
    return screen_width;
}

const int getDisplayHeight() {
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

const float getDisplayPixelRatio(bool _compute) {
    if (_compute && properties.style != EMBEDDED) {
#if defined(__EMSCRIPTEN__)
        return std::max(1.0f, float(emscripten_get_device_pixel_ratio()) * 0.65f);
#elif defined(DRIVER_GLFW)
        int window_width, window_height, framebuffer_width, framebuffer_height;
        glfwGetWindowSize(window, &window_width, &window_height);
        glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
        return std::max(1.0f, float(framebuffer_width))/std::max(1.0f, float(window_width));
#else
        return std::max(1.0f, device_pixel_ratio);
#endif
    }
    else
        return std::max(1.0f, device_pixel_ratio);
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

const glm::ivec4& getViewport() { return viewport; }
const glm::mat4& getOrthoMatrix() { return ortho_matrix; }
const glm::mat4& getFlippedOrthoMatrix() { return ortho_flipped_matrix; }
const int getWindowWidth() { return viewport.z * device_pixel_ratio; }
const int getWindowHeight() { return viewport.w * device_pixel_ratio; }
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

const double  getTime() { return elapsed_time;}
const double  getDelta() { return delta; }

void    setFps(int _fps) { 
    if (_fps == 0)
        rest_sec = 0.0;
    else
        rest_sec = 1.0f/(float)_fps; 
}
const double  getFps() { return 1.0/rest_sec; }

const float   getRestSec() { return rest_sec; }
const int     getRestMs() { return rest_sec * 1000; }
const int     getRestUs() { return rest_sec * 1000000; }

void    setMousePosition( float _x, float _y ) {
    mouse.pos = glm::vec2(_x, _y);
    mouse.vel = glm::vec2(0.0f, 0.0f);
    mouse.drag = glm::vec2(_x, _y);
    #if defined(DRIVER_GLFW)
    float h = getWindowHeight();
    float y = _y;//h - glm::clamp(_y, 0.0f, h);
    glfwSetCursorPos(window, _x / device_pixel_ratio , y / device_pixel_ratio);
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
