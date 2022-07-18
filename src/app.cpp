#include "vera/app.h"

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

    if (_app->auto_background_enabled)
        clear(_app->auto_background_color);

    _app->draw();
    renderGL();

    #if defined(__EMSCRIPTEN__)
    return (_app->xrMode == -1);
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

    post_setup = true;

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

        // mouseIsPressed = false;

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

        // mouseIsPressed = true;

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
#endif

#if defined(__EMSCRIPTEN__)
    // // Run the loop 
    emscripten_request_animation_frame_loop(loop, (void*)this);

    webxr_init(
        /* Frame callback */
        [](void* _userData, int, WebXRRigidTransform* _headPose, WebXRView* _views, int _viewCount) {
            App* _app = (App*)_userData;

            float px = getPixelDensity();
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

            if (_app->auto_background_enabled)
                clear(_app->auto_background_color);

            for(int viewIndex = 0; viewIndex < _viewCount; viewIndex++) {
                WebXRView view = _views[ viewIndex];
                glViewport( view.viewport[0], view.viewport[1], view.viewport[2], view.viewport[3] );

                Camera* cam = getCamera();
                if (cam == nullptr)
                    break;

                cam->setViewport(view.viewport[2], view.viewport[3]);

                // cam->setPosition( glm::make_vec3(view.viewPose.position) * -1.0f );
                
                glm::mat4 t = glm::translate(glm::mat4(1.), glm::make_vec3(view.viewPose.position) );
                glm::quat q = glm::quat(view.viewPose.orientation[3], view.viewPose.orientation[0], view.viewPose.orientation[1], view.viewPose.orientation[2]);
                glm::vec3 e = glm::eulerAngles(q);
                e.z = -e.z;
                glm::mat4 r = glm::toMat4( glm::quat(e) );
                
                cam->setTransformMatrix( glm::inverse(t * r) );
                cam->setProjection( glm::make_mat4(view.projectionMatrix) );

                _app->draw();
            }  

            renderGL();
        },
        /* Session start callback */
        [](void* _userData, int _mode) {
            std::cout << "Session START callback" << std::endl;
            App* app = (App*)_userData;
            app->xrMode = _mode;
        },
        /* Session end callback */
        [](void* _userData, int _mode) {
            std::cout << "Session END callback" << std::endl;
            ((App*)_userData)->xrMode = -1;
            emscripten_request_animation_frame_loop(loop, _userData);    
        },
        /* Error callback */
        [](void* _userData, int error) {
            std::cout << "Error: " << error << std::endl;
        },
        /* userData */
        this);
#else
    
    // Render Loop
    while ( isGL() )
        loop(getTime(), this);

    closeGL();
        close();

#endif
}

void App::background() { background(auto_background_color); }
void App::background( float _brightness ) { background(glm::vec3(_brightness)); }
void App::background( const glm::vec3& _color ) { background(glm::vec4(_color, 1.0f)); }
void App::background( const glm::vec4& _color ) {
    auto_background_color = _color;
    if (!post_setup)
        auto_background_enabled = true;
    clear(auto_background_color);
}

void App::orbitControl() {
    glEnable(GL_DEPTH_TEST);

    if (xrMode != -1)
        return;

    Camera* cam = getCamera();

    if (cam == nullptr)
        return;

    double aspect = width/height;
    if (cam->getAspect() != aspect)
        cam->setViewport(width, height);

    if (mouseIsPressed) {
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
            dist += (-.008f * movedY);
            if (dist > 0.0f) {
                cam->orbit(cameraLat, cameraLon, dist);
                cam->lookAt(glm::vec3(0.0));
            }
        }
    }
}

}