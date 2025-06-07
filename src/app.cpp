#include "vera/app.h"

#include "vera/ops/fs.h"
#include "vera/ops/pixel.h"
#include "vera/ops/time.h"

#include <iostream>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
// #define GLFW_INCLUDE_ES3

#include <glm/gtc/type_ptr.hpp>
#include "webxr.h"
#endif

namespace vera {

#if defined(__EMSCRIPTEN__)
EM_BOOL App::loop (double _time, void* _userData) {
    App* _app = (App*)_userData;
#else
void App::loop(double _time, App* _app) {
#endif
    
    _app->time = _time;
    _app->width = getWindowWidth();
    _app->height = getWindowHeight();
    _app->focused = getMouseEntered();
    _app->deltaTime = getDelta();
    _app->frameCount++;

    // Update
    _app->update();
    updateGL();

    if (_app->m_saveToPath.length() > 0) {
        if (!_app->m_framebuffer.isAllocated())
            _app->m_framebuffer.allocate(_app->width, _app->height, vera::COLOR_TEXTURE_DEPTH_BUFFER);

        if (vera::haveExt(_app->m_saveToPath, "png")) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        }
        _app->m_framebuffer.bind();
    }

    if (vera::getBackgroundEnabled())
        vera::background();

    if (vera::getWindowStyle() == vera::LENTICULAR) {
        vera::renderQuilt([&](const vera::QuiltProperties& quilt, glm::vec4& viewport, int &viewIndex) {
            _app->draw();
        });
    }
    else
        _app->draw();
        
    renderGL();

    if (_app->m_saveToPath.length() > 0) {
        _app->m_framebuffer.unbind();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA);

        vera::image(_app->m_framebuffer);

        _app->onSave();
    }

    #if defined(__EMSCRIPTEN__)
    return (getXR() == NONE_XR_MODE);
    #endif
}

void App::run(WindowProperties _properties) {
    initGL(_properties);

    width = getWindowWidth();
    height = getWindowHeight();
    deltaTime = getDelta();
    time = getTime();
    frameCount = 0;

    mouseX = 0.0f;
    mouseY = 0.0f;
    movedX = 0.0f;
    movedY = 0.0f;
    pmouseX = 0.0f;
    pmouseY = 0.0f;

    setup();

    bPostSetup = true;

#ifdef EVENTS_AS_CALLBACKS
    setViewportResizeCallback( [&](int _width, int _height) {
        onViewportResize(_width, _height); 
        windowResized();
    } );
    
    setKeyPressCallback( [&](int _key) { 
        onKeyPress(_key); 
        keyPressed();
    } );

    setMouseMoveCallback( [&](float _x, float _y) { 
        mouseX = _x;
        mouseY = _y;
        
        movedX = getMouseVelX();
        movedY = getMouseVelY();

        pmouseX = _x - movedX;
        pmouseY = _y - movedY;

        onMouseMove(_x, _y); 
        mouseMoved();
    } );
    
    setMousePressCallback( [&](float _x, float _y, int _button) { 
        mouseButton = _button;
        mouseIsPressed = true;

        onMousePress(_x, _y, _button); 
        mousePressed();
        mouseClicked();
    } );

    setMouseDragCallback( [&](float _x, float _y, int _button) { 
        mouseButton = _button;

        mouseX = _x;
        mouseY = _y;
        
        movedX = getMouseVelX();
        movedY = getMouseVelY();

        pmouseX = _x - movedX;
        pmouseY = _y - movedY;

        mouseDragged();
        onMouseDrag(_x, _y, _button); 
    } );

    setMouseReleaseCallback( [&](float _x, float _y, int _button) { 
        mouseButton = 0;
        mouseIsPressed = false;

        onMouseRelease(_x, _y, _button);
        mouseReleased();
    } );

    setScrollCallback( [&](float _yoffset) { 
        onScroll(_yoffset); 
    } );

    setDropCallback( [&](int _count, const char** _paths) {
        onDrop(_count, _paths);
    } );

#endif

#if defined(__EMSCRIPTEN__)
    // // Run the loop 
    emscripten_request_animation_frame_loop(loop, (void*)this);

    webxr_init(
        /* Frame callback */
        [](void* _userData, int, WebXRRigidTransform* _headPose, WebXRView* _views, int _viewCount) {
            App* _app = (App*)_userData;

            float px = getDisplayPixelRatio();
            float width = getWindowWidth();
            float height = getWindowHeight();

            _app->time = getTime();
            _app->width = getWindowWidth();
            _app->height = getWindowHeight();
            _app->focused = getMouseEntered();
            _app->deltaTime = getDelta();
            _app->frameCount++;

            // Update
            _app->update();
            updateGL();

            if (_app->m_backgroundEnabled)
                clear(_app->m_backgroundColor);

            Camera* cam = getCamera();

            if (cam == nullptr)
                return;

            webxr_set_projection_params(cam->getNearClip(), cam->getFarClip());

            glm::vec3 cam_pos = cam->getPosition();
            glm::vec3 head_pos = glm::make_vec3(_headPose->position);
            for(int viewIndex = 0; viewIndex < _viewCount; viewIndex++) {
                WebXRView view = _views[ viewIndex];
                glViewport( view.viewport[0], view.viewport[1], view.viewport[2], view.viewport[3] );
                cam->setViewport(view.viewport[2], view.viewport[3]);
                glm::mat4 t = glm::translate(glm::mat4(1.), glm::make_vec3(view.viewPose.position) + head_pos );
                glm::mat4 r = glm::toMat4( glm::quat(view.viewPose.orientation[3], view.viewPose.orientation[0], view.viewPose.orientation[1], view.viewPose.orientation[2]) );
                // cam->setTransformMatrix( glm::translate( glm::inverse(t * r), cam_pos) );
                cam->setTransformMatrix( glm::inverse(t * r) );
                cam->setProjection( glm::make_mat4(view.projectionMatrix) );
                _app->draw();
            } 

            renderGL();

            cam->setPosition(cam_pos);
        },
        /* Session start callback */
        [](void* _userData, int _mode) {
            std::cout << "Session START XR mode " << _mode << std::endl;
            setXR((XrMode)_mode);

            // // TODO: select START/END callbacks
            // webxr_set_select_start_callback([](WebXRInputSource *_inputSource, void *_userData) { 
            //     printf("select_start_callback\n"); 
            // }, _userData);

            // webxr_set_select_end_callback([](WebXRInputSource *_inputSource, void *_userData) { 
            //     printf("select_end_callback\n");
            // }, _userData);
            
        },
        /* Session end callback */
        [](void* _userData, int _mode) {
            std::cout << "Session END callback" << std::endl;
            setXR(NONE_XR_MODE);
            emscripten_request_animation_frame_loop(loop, _userData);    
        },
        /* Error callback */
        [](void* _userData, int _error) {
            switch (_error){
                case WEBXR_ERR_API_UNSUPPORTED:
                    std::cout << "WebXR unsupported in this browser." << std::endl;
                    break;
                case WEBXR_ERR_GL_INCAPABLE:
                    std::cout << "GL context cannot be used to render to WebXR" << std::endl;
                    break;
                case WEBXR_ERR_SESSION_UNSUPPORTED:
                    std::cout << "VR not supported on this device" << std::endl;
                    break;
                default:
                    std::cout << "Unknown WebXR error with code" << std::endl;
                }
        },
        /* userData */
        this);

        webxr_is_session_supported(WEBXR_SESSION_MODE_IMMERSIVE_VR, [](int _mode, int _supported) {
            if ((_mode == WEBXR_SESSION_MODE_IMMERSIVE_VR) && (_supported)) {
                webxr_request_session(WEBXR_SESSION_MODE_IMMERSIVE_VR, WEBXR_SESSION_FEATURE_LOCAL, WEBXR_SESSION_FEATURE_LOCAL);
            } 
        });
#else
    
    // Render Loop
    while ( isGL() && !bShouldExit)
        loop(getTime(), this);

    closeGL();
    close();

#endif
}

void App::save(const std::string& _path, bool _exitAfterSave) { 
    m_saveToPath = _path;
    if (m_saveToPath.empty()) {
        m_saveToPath = "screenshot_" + vera::getDateTimeString() + ".png";
    }
    m_exitAfterSave = _exitAfterSave;
}

void App::onSave() {

    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer.getId());

    if (vera::getExt(m_saveToPath) == "hdr") {
        float* pixels = new float[vera::getWindowWidth() * vera::getWindowHeight()*4];
        glReadPixels(0, 0, vera::getWindowWidth(), vera::getWindowHeight(), GL_RGBA, GL_FLOAT, pixels);
        vera::savePixelsFloat(m_saveToPath, pixels, vera::getWindowWidth(), vera::getWindowHeight());
    }
    else {
        int width = vera::getWindowWidth();
        int height = vera::getWindowHeight();
        auto pixels = std::unique_ptr<unsigned char[]>(new unsigned char [width * height * 4]);
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.get());
        vera::savePixels(m_saveToPath, pixels.get(), width, height);
    }

    std::cout << "Saved to " << m_saveToPath << std::endl;
    m_saveToPath = "";
    
    if (m_exitAfterSave) {
        bShouldExit = true;
    }
}

void App::orbitControl() {
    if (getXR() > 0)
        return;

    Camera* cam = getCamera();

    if (cam == nullptr)
        return;

    if (cam->getViewport() == glm::ivec4(0.0)) {
        double aspect = width/height;
        if (cam->getAspect() != aspect)
            cam->setViewport(width, height);
    }

    if (mouseIsPressed && getQuiltCurrentViewIndex() == 0) {
        float dist = cam->getDistance();

        if (mouseButton == 1) {

            // Left-button drag is used to rotate geometry.
            if (fabs(movedX) < 50.0 && fabs(movedY) < 50.0) {
                cameraLat -= getMouseVelX();
                cameraLon -= getMouseVelY() * 0.5;
                cam->orbit(cameraLat, cameraLon, dist);
                cam->lookAt(glm::vec3(0.0));
            }
        } 
        else if (mouseButton == 2) {

            // Right-button drag is used to zoom geometry.
            dist += (-.05f * movedY);
            if (dist > 0.0f) {
                cam->orbit(cameraLat, cameraLon, dist);
                cam->lookAt(glm::vec3(0.0));
            }
        }
    }

    if (vera::getWindowStyle() == vera::LENTICULAR)
        cam->setVirtualOffset(1.5, getQuiltCurrentViewIndex(), getQuiltTotalViews());

    setDepthTest(true);
}

}