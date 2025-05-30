#include "vera/gl/gl.h"

namespace vera {

static bool         bDepthTest = false;
static BlendMode    sBlendMode = BLEND_ALPHA;
static CullingMode  sCullingMode = CULL_BACK;

#if defined(PLATFORM_RPI)

    #ifndef DRIVER_BROADCOM
    static PFNEGLCREATEIMAGEKHRPROC createImageProc = NULL;
    static PFNEGLDESTROYIMAGEKHRPROC destroyImageProc = NULL;
    static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC imageTargetTexture2DProc = NULL;
    #endif

    EGLImageKHR createImage(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list) {

        #ifdef DRIVER_BROADCOM
        return eglCreateImageKHR(dpy, ctx, target, buffer, attrib_list);

        #else
        if (!createImageProc)
            createImageProc = (PFNEGLCREATEIMAGEKHRPROC) eglGetProcAddress("eglCreateImageKHR");

        return createImageProc(dpy, ctx, target, buffer, attrib_list);
        #endif
    }

    EGLBoolean destroyImage(EGLDisplay dpy, EGLImageKHR image) {

        #ifdef DRIVER_BROADCOM
        return eglDestroyImageKHR(dpy, image);

        #else
        if (!destroyImageProc)
            destroyImageProc = (PFNEGLDESTROYIMAGEKHRPROC) eglGetProcAddress("eglDestroyImageKHR");
        
        return destroyImageProc(dpy, image);
        #endif
    }

    void imageTargetTexture2D(EGLenum target, EGLImageKHR image) {

        #ifdef DRIVER_BROADCOM
        return glEGLImageTargetTexture2DOES(target, image);

        #else
        if (!imageTargetTexture2DProc)
            imageTargetTexture2DProc = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) eglGetProcAddress("glEGLImageTargetTexture2DOES");
        
        imageTargetTexture2DProc(target, image);
        #endif
    }

#endif

void blendMode( BlendMode _mode ) {
    sBlendMode = _mode;
    switch (_mode) {
        case BLEND_ALPHA:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;

        case BLEND_ADD:
            glEnable(GL_BLEND);
            glBlendEquation(GL_FUNC_ADD);
            // glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glBlendFunc(GL_ONE, GL_ONE);
            break;

        case BLEND_MULTIPLY:
            glEnable(GL_BLEND);
            glBlendEquation(GL_FUNC_ADD);
            glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA );
            break;

        case BLEND_SCREEN:
            glEnable(GL_BLEND);
            glBlendEquation(GL_FUNC_ADD);
            glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE);
            break;

        case BLEND_SUBSTRACT:
            glEnable(GL_BLEND);
            glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;

        case BLEND_NONE:
            glDisable(GL_BLEND);
            break;

        default:
            break;
    }
}
const BlendMode blendMode() { return sBlendMode; }

void cullingMode(CullingMode _mode) {
    sCullingMode = _mode;
    switch (_mode) {
        case CULL_FRONT:glDisable(GL_CULL_FACE); glCullFace(GL_FRONT); break;
        case CULL_BACK: glDisable(GL_CULL_FACE); glCullFace(GL_BACK); break;
        case CULL_BOTH: glDisable(GL_CULL_FACE); glCullFace(GL_FRONT_AND_BACK); break;
        case CULL_NONE: glDisable(GL_CULL_FACE); break;
        default: break;
    }
}
const CullingMode cullingMode() { return sCullingMode; }

void setDepthTest(bool _value) {
    bDepthTest = _value;
    if (_value)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
}

const bool getDepthTest() { return bDepthTest; }


}
