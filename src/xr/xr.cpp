#include "vera/xr/xr.h"

namespace vera {

static XrMode currentMode = NONE_XR_MODE;

void setXR(XrMode _mode) { currentMode = _mode; }
XrMode getXR() { return currentMode; }

#if defined(__EMSCRIPTEN__)

void requestXR(XrMode _mode) {
    if (_mode == 1) {
        webxr_is_session_supported(WEBXR_SESSION_MODE_IMMERSIVE_VR, [](int _mode, int _supported) {
            if (_supported)
                webxr_request_session(WEBXR_SESSION_MODE_IMMERSIVE_VR, WEBXR_SESSION_FEATURE_LOCAL, WEBXR_SESSION_FEATURE_LOCAL);
        });
    }
    else if (_mode == 2) {
        webxr_is_session_supported(WEBXR_SESSION_MODE_IMMERSIVE_AR, [](int _mode, int _supported) {
            if (_supported)
                webxr_request_session(WEBXR_SESSION_MODE_IMMERSIVE_AR, WEBXR_SESSION_FEATURE_LOCAL, WEBXR_SESSION_FEATURE_LOCAL);
        });
    }
}

void setCameraForXRView(Camera* _cam, WebXRView* _view, glm::vec3 _offset) {
    _cam->setViewport(_view->viewport[2], _view->viewport[3]);
    glm::mat4 t = glm::translate(glm::mat4(1.), glm::make_vec3(_view->viewPose.position) + _offset );
    glm::mat4 r = glm::toMat4( glm::quat(_view->viewPose.orientation[3], _view->viewPose.orientation[0], _view->viewPose.orientation[1], _view->viewPose.orientation[2]) );
    _cam->setTransformMatrix( glm::inverse(t * r) );
    _cam->setProjection( glm::make_mat4(_view->projectionMatrix) );
}
#endif

}