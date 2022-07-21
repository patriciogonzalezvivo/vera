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

#endif

}