#pragma once

#include "node.h"
#include "../gl/fbo.h"

namespace vera {

enum LightType {
    LIGHT_DIRECTIONAL, LIGHT_POINT, LIGHT_SPOT
};

class Light : public Node {
public:
    Light();
    Light(glm::vec3 _dir);
    Light(glm::vec3 _pos, float _falloff);
    Light(glm::vec3 _pos, glm::vec3 _dir, float _falloff = -1.0);
    virtual ~Light();

    // virtual void        setPosition(const glm::vec3& _pos);
    void                setType(LightType _type) { m_lightType = _type; }
    void                setColor(float _r, float _g, float _b) { color = glm::vec3(_r, _g, _b); }
    void                setDirection(float _x, float _y, float _z) { direction = glm::vec3(_x, _y, _z); }
    void                setIntensity(float _intensity) { intensity = _intensity; }
    void                setFallOff(float _falloff) { falloff = _falloff; }

    const LightType&    getLightType() const { return m_lightType; }

    const glm::mat4&    getMVPMatrix( const glm::mat4 &_model, float _area);
    const glm::mat4&    getViewMatrix() const { return m_viewMatrix; };
    const glm::mat4&    getProjectionMatrix() const { return m_projectionMatrix; };
    const glm::mat4&    getBiasMVPMatrix() const { return m_mvp_biased; };

    const Fbo*          getShadowMap() const { return &m_shadowMap; }
    float               getShadowMapNear() const { return m_near; }
    float               getShadowMapFar() const { return m_far; }

    void                bindShadowMap();
    void                unbindShadowMap();

    glm::vec3           color;
    glm::vec3           direction;
    float               intensity;
    float               falloff;

protected:
    virtual void        onPositionChanged() { bChange = true; };
    virtual void        onOrientationChanged() { bChange = true; };
    virtual void        onScaleChanged() { bChange = true; };

    Fbo                 m_shadowMap;
    glm::mat4           m_viewMatrix;
    glm::mat4           m_projectionMatrix;
    glm::mat4           m_mvp;
    glm::mat4           m_mvp_biased;

    float               m_near;
    float               m_far;

    LightType           m_lightType;
    
    GLint               m_viewport[4];
};

}