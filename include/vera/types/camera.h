#pragma once

#include "node.h"
#include "boundingBox.h"

#include <map>
#include <vector>

namespace vera {

enum ProjectionType {
    ORTHO, 
    PERSPECTIVE,
    PERSPECTIVE_VIRTUAL_OFFSET,
    CUSTOM
};

class Camera: public Node {
public:
    Camera();
    virtual ~Camera();

    virtual void        setProjection(ProjectionType cam_mode);
    virtual void        setProjection(const glm::mat4& _M );
    virtual void        setProjection(  float x1, float y1, float z1, float w1,
                                        float x2, float y2, float z2, float w2,
                                        float x3, float y3, float z3, float w3,
                                        float x4, float y4, float z4, float w4) {
                            setProjection(glm::mat4(x1, y1, z1, w1, x2, y2, z2, w2, x3, y3, z3, w3, x4, y4, z4, w4));
                        }

    virtual void        setFOV(double _fov);
    virtual void        setAspect(float _aspect) { m_aspect = _aspect; }
    virtual void        setViewport(int _width, int _height);
    virtual void        setViewport(glm::vec4 _viewport);
    virtual void        setClipping(double _near_clip_distance, double _far_clip_distance);

    virtual void        setVirtualOffset(float _scale, int _currentViewIndex, int _totalViews, float aspect = 1.0f);

    virtual void        setExposure(float _aperture, float _shutterSpeed, float _sensitivity);

    virtual glm::vec3   worldToCamera(const glm::vec3* _world, glm::mat4* _model = nullptr) const;
    virtual glm::vec3   worldToCamera(const glm::vec3& _world, glm::mat4* _model = nullptr) const;
    virtual glm::vec3   worldToScreen(const glm::vec3* _world, glm::mat4* _model = nullptr) const;
    virtual glm::vec3   worldToScreen(const glm::vec3& _world, glm::mat4* _model = nullptr) const;
    virtual BoundingBox worldToScreen(const BoundingBox& _bbox, glm::mat4* _model = nullptr) const;

    //Getting Functions
    virtual const ProjectionType&   getProjectionType() const { return m_projectionType;};

    virtual const float         getFOV() const { return m_fov; }
    virtual const float         getAspect() const { return m_aspect; }
    virtual const float         getFarClip() const { return m_farClip; }
    virtual const float         getNearClip() const { return m_nearClip; }

    virtual const float         getEv100() const { return m_ev100; }
    virtual const double        getExposure() const { return m_exposure; }
    virtual const float         getAperture() const { return m_aperture; }          //! returns this camera's aperture in f-stops
    virtual const float         getShutterSpeed() const { return m_shutterSpeed; }  //! returns this camera's shutter speed in seconds
    virtual const float         getSensitivity() const { return m_sensitivity; }    //! returns this camera's sensitivity in ISO
    
    virtual const glm::vec3&    getPosition() const;
    virtual const glm::mat4&    getViewMatrix() const;
    virtual const glm::mat3&    getNormalMatrix() const { return m_normalMatrix; }
    virtual const glm::mat4&    getInverseViewMatrix() const { return m_inverseViewMatrix; }
    virtual const glm::mat4&    getProjectionMatrix() const { return m_projectionMatrix; }
    virtual const glm::mat4&    getProjectionViewMatrix() const { return m_projectionViewMatrix; }
    virtual const glm::mat4&    getInverseProjectionMatrix() const { return m_inverseProjectionMatrix; }

    virtual glm::ivec4          getViewport() const;

    // Camera Controls
    virtual void                setTarget(glm::vec3 _target);
    virtual const glm::vec3&    getTarget() const { return m_target; }
    virtual float               getDistance() const { return glm::length(m_position + m_target); }

    virtual void                moveTarget(float x, float y); // move target in camera's local space (screen-relative)
    virtual void                moveTarget(const glm::vec3& delta) { setTarget(m_target + delta); } // move target in world space
    virtual void                orbit(float _azimuth, float _elevation, float _distance);

    virtual void                begin();
    virtual void                end();


    bool                        bFlipped = false;
protected:
    virtual void        onPositionChanged();
    virtual void        onOrientationChanged();
    virtual void        onScaleChanged();

    virtual void        updateCameraSettings();
    virtual void        updateProjectionViewMatrix();

private:
    glm::mat4   m_viewMatrix;
    glm::mat4   m_inverseViewMatrix;
    
    glm::mat3   m_normalMatrix;
    
    glm::mat4   m_projectionMatrix;
    glm::mat4   m_projectionViewMatrix;
    glm::mat4   m_inverseProjectionMatrix;

    glm::ivec4  m_viewport;
    glm::ivec4  m_viewport_old;
    
    glm::vec3   m_position_offset;

    glm::vec3   m_target;

    float       m_aspect;
    float       m_fov;
    float       m_nearClip;
    float       m_farClip;

    double      m_exposure; 
    float       m_ev100;
    float       m_aperture;
    float       m_shutterSpeed;
    float       m_sensitivity;

    ProjectionType m_projectionType;
};

typedef std::shared_ptr<Camera>         CameraPtr;
typedef std::shared_ptr<const Camera>   CameraConstPtr;
typedef std::map<std::string, Camera*>  CamerasMap;

}