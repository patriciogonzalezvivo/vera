#include "vera/types/camera.h"

#include "vera/window.h"
#include "vera/ops/math.h"

#include <iostream>

#include "glm/gtc/matrix_inverse.hpp"

// static const float MIN_APERTURE = 0.5f;
// static const float MAX_APERTURE = 64.0f;
// static const float MIN_SHUTTER_SPEED = 1.0f / 25000.0f;
// static const float MAX_SHUTTER_SPEED = 60.0f;
// static const float MIN_SENSITIVITY = 10.0f;
// static const float MAX_SENSITIVITY = 204800.0f;

namespace vera {

Camera::Camera(): 
    m_viewMatrix(1.0f), m_inverseViewMatrix(1.0f),
    m_normalMatrix(1.0f), 
    m_projectionMatrix(1.0f), m_projectionViewMatrix(1.0f), m_inverseProjectionMatrix(1.0f),
    m_viewport(0), m_viewport_old(0),
    m_target(0.0), m_position_offset(0.0),
    m_aspect(4.0f/3.0f), m_fov(45.), m_nearClip(0.01f), m_farClip(1000.0f), 
    m_exposure(2.60417e-05), m_ev100(14.9658), m_aperture(16), m_shutterSpeed(1.0f/125.0f), m_sensitivity(100.0f), 
    m_projectionType(ProjectionType::PERSPECTIVE) {

    updateCameraSettings();
}

Camera::~Camera() {

}

void Camera::setViewport(int _width, int _height){
    m_aspect = double(_width) / double(_height);
    updateCameraSettings();
}

void Camera::setViewport(glm::vec4 _viewport) {
    m_viewport = _viewport;
    setViewport(_viewport.z, _viewport.w);
}

//Setting Functions
void Camera::setProjection(ProjectionType _type) {
    m_projectionType = _type;
    m_viewMatrix = getTransformMatrix();
    lookAt(m_target);
    updateCameraSettings();
}

void Camera::setFOV(double _fov) {
    m_fov = _fov;
    updateCameraSettings();
}

void Camera::setClipping(double _near_clip_distance, double _far_clip_distance) {
    m_nearClip = _near_clip_distance;
    m_farClip = _far_clip_distance;
    updateCameraSettings();
}

void Camera::setTarget(glm::vec3 _target) {
    m_target = _target;
    bChange = true;
}

void Camera::setDistance(float _distance) {
    setPosition( -_distance * getZAxis() );
    lookAt(m_target);
}

void Camera::setProjection(const glm::mat4& _M ) {
    m_projectionType = ProjectionType::CUSTOM;
    m_projectionMatrix = _M;
    bChange = true;
}

void Camera::setVirtualOffset(float scale, int currentViewIndex, int totalViews, float aspect) {
    // The standard model Looking Glass screen is roughly 4.75" vertically. If we
    // assume the average viewing distance for a user sitting at their desk is
    // about 36", our field of view should be about 14°. There is no correct
    // answer, as it all depends on your expected user's distance from the Looking
    // Glass, but we've found the most success using this figure.

    // start at -viewCone * 0.5 and go up to viewCone * 0.5
    const float viewCone = glm::radians(40.0);   // view cone of hardware, always around 40
    float offsetAngle = (float(currentViewIndex) / (float(totalViews) - 1.0f) - 0.5f) * viewCone;

    // calculate the offset that the camera should move
    float offset = -getDistance() * tan(offsetAngle) * 0.5f;

    // modify the view matrix (position)
    // determine the local direction of the offset 
    glm::vec3 offsetLocal = getXAxis() * offset;
    m_viewMatrix = glm::translate(getTransformMatrix(), offsetLocal);
    m_normalMatrix = glm::transpose(glm::inverse(glm::mat3(m_viewMatrix)));

    glm::mat4 projectionMatrix = glm::perspective(m_fov, getAspect(), getNearClip(), getFarClip());

    // modify the projection matrix, relative to the camera size and aspect ratio
    projectionMatrix[2][0] += offset / (scale * aspect);

    m_position_offset = -glm::vec3(glm::inverse(m_viewMatrix)[3]);
    m_projectionMatrix = projectionMatrix;
    m_projectionViewMatrix = projectionMatrix * m_viewMatrix;

    m_inverseViewMatrix = vera::inverseMatrix(m_viewMatrix);
    m_inverseProjectionMatrix = vera::inverseMatrix(m_projectionMatrix);
    
    bChange = true;
    // updateProjectionViewMatrix();
}

const glm::mat4& Camera::getViewMatrix() const {
    if (m_projectionType == ProjectionType::PERSPECTIVE_VIRTUAL_OFFSET )
        return m_viewMatrix;
    else 
        return getTransformMatrix(); 
}

glm::ivec4 Camera::getViewport() const {
    if (m_viewport == glm::ivec4(0))
        return glm::ivec4(0, 0, vera::getWindowWidth(), vera::getWindowHeight());
    else
        return m_viewport;
}

const glm::vec3& Camera::getPosition() const {
    if (m_projectionType == ProjectionType::PERSPECTIVE_VIRTUAL_OFFSET )
        return m_position_offset;
    else 
        return m_position;
}

const float Camera::getDistance() const { 
    return glm::length(m_position);
}

void Camera::begin() {
    setDepthTest(true);
    
    if (bChange) {
        updateCameraSettings();
        bChange = false;
    }

    // If the viewport is set to zero, we don't want to change it
    if (m_viewport == glm::ivec4(0)) {
        setViewport(vera::getWindowWidth(), vera::getWindowHeight());
        return;
    }
    
    // extract current viewport so we can restore it later
    int vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    m_viewport_old = glm::ivec4(vp[0], vp[1], vp[2], vp[3]);

    // set new viewport
    glViewport(m_viewport.x, m_viewport.y, m_viewport.z, m_viewport.w);
}

void Camera::end() {
    // If the viewport is set to zero, it was never changed so we don't want to change it back
    if (m_viewport == glm::ivec4(0))
        return;

    // restore previous viewport
    glViewport(m_viewport_old.x, m_viewport_old.y, m_viewport_old.z, m_viewport_old.w);
}


/** Sets this camera's exposure (default is 16, 1/125s, 100 ISO)
 * from https://github.com/google/filament/blob/master/filament/src/Exposure.cpp
 *
 * The exposure ultimately controls the scene's brightness, just like with a real camera.
 * The default values provide adequate exposure for a camera placed outdoors on a sunny day
 * with the sun at the zenith.
 *
 * @param aperture      Aperture in f-stops, clamped between 0.5 and 64.
 *                      A lower \p aperture value *increases* the exposure, leading to
 *                      a brighter scene. Realistic values are between 0.95 and 32.
 *
 * @param shutterSpeed  Shutter speed in seconds, clamped between 1/25,000 and 60.
 *                      A lower shutter speed increases the exposure. Realistic values are
 *                      between 1/8000 and 30.
 *
 * @param sensitivity   Sensitivity in ISO, clamped between 10 and 204,800.
 *                      A higher \p sensitivity increases the exposure. Realistice values are
 *                      between 50 and 25600.
 *
 * @note
 * With the default parameters, the scene must contain at least one Light of intensity
 * similar to the sun (e.g.: a 100,000 lux directional light).
 *
 * @see Light, Exposure
 */

void  Camera::setExposure(float _aperture, float _shutterSpeed, float _sensitivity) {
    m_aperture = _aperture;
    m_shutterSpeed = _shutterSpeed;
    m_sensitivity = _sensitivity;
    
    // With N = aperture, t = shutter speed and S = sensitivity,
    // we can compute EV100 knowing that:
    //
    // EVs = log2(N^2 / t)
    // and
    // EVs = EV100 + log2(S / 100)
    //
    // We can therefore find:
    //
    // EV100 = EVs - log2(S / 100)
    // EV100 = log2(N^2 / t) - log2(S / 100)
    // EV100 = log2((N^2 / t) * (100 / S))
    //
    // Reference: https://en.wikipedia.org/wiki/Exposure_value
    m_ev100 = std::log2((_aperture * _aperture) / _shutterSpeed * 100.0f / _sensitivity);

    // This is equivalent to calling exposure(ev100(N, t, S))
    // By merging the two calls we can remove extra pow()/log2() calls
    const float e = (_aperture * _aperture) / _shutterSpeed * 100.0f / _sensitivity;
    m_exposure = 1.0f / (1.2f * e);

    bChange = true;
};

// ---------------------------------------------------------- Convertions

void Camera::updateCameraSettings() {
    setExposure(getAperture(), getShutterSpeed(), getSensitivity());
    
    if (m_projectionType == ProjectionType::ORTHO)
        m_projectionMatrix = glm::ortho(-1.5f * float(m_aspect), 1.5f * float(m_aspect), -1.5f, 1.5f, -10.0f, 10.f);
    else if (m_projectionType != ProjectionType::CUSTOM)
        m_projectionMatrix = glm::perspective(m_fov, m_aspect, m_nearClip, m_farClip);
    
    updateProjectionViewMatrix();
}

void Camera::updateProjectionViewMatrix() {
    m_projectionViewMatrix = m_projectionMatrix * getViewMatrix();
    m_normalMatrix = glm::transpose(glm::inverse(glm::mat3(getViewMatrix())));

    m_inverseViewMatrix = vera::inverseMatrix(getViewMatrix());
    m_inverseProjectionMatrix = vera::inverseMatrix(m_projectionMatrix);
    
    bChange = true;
}

glm::vec3 Camera::worldToCamera(const glm::vec3& _world, glm::mat4* _model) const { return worldToCamera(&_world, _model); }
glm::vec3 Camera::worldToCamera(const glm::vec3* _world, glm::mat4* _model) const {
    glm::vec4 pos = glm::vec4(*_world, 1.0f);
    if (_model != nullptr)
        pos = (*_model) * pos;
    glm::vec4 camera = m_projectionViewMatrix * pos;
    return glm::vec3(camera) / camera.w;
}

glm::vec3 Camera::worldToScreen(const glm::vec3& _world, glm::mat4* _model) const { return worldToScreen(&_world, _model); }
glm::vec3 Camera::worldToScreen(const glm::vec3* _world, glm::mat4* _model) const { 
    glm::vec3 screen = worldToCamera(_world, _model);
    screen.x = (screen.x + 1.0f) * 0.5f;
    screen.y = (1.0f - screen.y) * 0.5f;
    // screen.x *= vera::getWindowWidth();
    // screen.y *= vera::getWindowHeight();
    return screen;
}

BoundingBox Camera::worldToScreen(const BoundingBox& _bbox, glm::mat4* _model) const {
    BoundingBox bbox;
    bbox.expand( worldToScreen(_bbox.min, _model) );
    bbox.expand( worldToScreen(glm::vec3( _bbox.min.x, _bbox.min.y, _bbox.max.z ) , _model) );
    bbox.expand( worldToScreen(glm::vec3( _bbox.min.x, _bbox.max.y, _bbox.max.z ) , _model) );
    bbox.expand( worldToScreen(glm::vec3( _bbox.max.x, _bbox.max.y, _bbox.min.z ) , _model) );
    bbox.expand( worldToScreen(glm::vec3( _bbox.max.x, _bbox.min.y, _bbox.min.z ) , _model) );
    bbox.expand( worldToScreen(_bbox.max, _model) );
    return bbox;
}

// ---------------------------------------------------------- Events

void Camera::onPositionChanged() { updateProjectionViewMatrix(); }
void Camera::onOrientationChanged() { updateProjectionViewMatrix(); }
void Camera::onScaleChanged() { updateProjectionViewMatrix(); }

}