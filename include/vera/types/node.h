#pragma once

#include <vector>

#include "glm/glm.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

namespace vera {

enum NodeType {
    EMPTY_NODE = 0, 
    MODEL_NODE,
    LIGHT_NODE,
    CAMERA_NODE
};

class Node {
public:

    Node();
    virtual ~Node() { };

    // SET
    virtual void        setProperties(const Node& _other);

    virtual void        setScale(float s) { setScale(glm::vec3(s, s, s));}
    virtual void        setScale(const glm::vec3& _scale);
    virtual void        setScale(float x, float y, float z) { setScale(glm::vec3(x,y,z)); }

    virtual void        setPosition(const glm::vec3& _pos);
    virtual void        setPosition(float x, float y, float z) { setPosition(glm::vec3(x,y,z)); }
    
    virtual void        setOrientation(const glm::vec3& _ori);
    virtual void        setOrientation(float x, float y, float z) { setOrientation(glm::vec3(x,y,z)); }
    virtual void        setOrientation(const glm::quat& _ori);
    virtual void        setOrientation(float x, float y, float z, float w) { setOrientation(glm::quat(x,y,z,w)); }
    virtual void        setOrientation(const glm::mat3& _ori);
    virtual void        setOrientation( float x1, float y1, float z1,
                                        float x2, float y2, float z2,
                                        float x3, float y3, float z3) {
                                            setOrientation(glm::mat3( x1, y1, z1, x2, y2, z2, x3, y3, z3 ));
                                        }
    virtual void        setOrientation(const glm::mat4& _ori);
    virtual void        setOrientation( float x1, float y1, float z1, float w1,
                                        float x2, float y2, float z2, float w2,
                                        float x3, float y3, float z3, float w3,
                                        float x4, float y4, float z4, float w4) {
                                            setOrientation(glm::mat4(x1, y1, z1, w1, x2, y2, z2, w2, x3, y3, z3, w3, x4, y4, z4, w4));
                                        }
    
    virtual void        setTransformMatrix(const glm::mat4& _m);
    virtual void        setTransformMatrix( float x1, float y1, float z1, float w1,
                                            float x2, float y2, float z2, float w2,
                                            float x3, float y3, float z3, float w3,
                                            float x4, float y4, float z4, float w4) {
                                            setTransformMatrix(glm::mat4(x1, y1, z1, w1, x2, y2, z2, w2, x3, y3, z3, w3, x4, y4, z4, w4));
                                        }

    // GET 
    // virtual NodeType            getType() const { return m_type; }
    virtual const glm::vec3&    getScale() const { return m_scale; };
    virtual const glm::vec3&    getPosition() const { return m_position; };

    virtual const glm::vec3&    getXAxis() const { return m_axis[0]; }
    virtual const glm::vec3&    getYAxis() const { return m_axis[1]; };
    virtual const glm::vec3&    getZAxis() const { return m_axis[2]; };

    virtual const glm::vec3&    getRightDir() const { return getXAxis(); }
    virtual const glm::vec3&    getUpDir() const { return getYAxis(); }
    virtual glm::vec3           getForwardDir() const { return -getZAxis(); }

    virtual const glm::quat&    getOrientationQuat() const { return m_orientation; }
    virtual glm::vec3           getOrientationEuler() const { return glm::eulerAngles(m_orientation); }
    virtual glm::mat4           getOrientationMatrix() const { return glm::toMat4(m_orientation); };
    
    virtual float               getPitch() const { return getOrientationEuler().x; }
    virtual float               getHeading() const { return getOrientationEuler().y; }
    virtual float               getRoll() const { return getOrientationEuler().z; };

    virtual const glm::mat4&    getTransformMatrix() const { return m_transformMatrix; };

    // ACTIONS
    virtual void        scale(float s) { scale(glm::vec3(s, s, s)); }
    virtual void        scale(const glm::vec3& _scale);
    virtual void        scale(float x, float y, float z) { scale(glm::vec3(x,y,z)); }

    virtual void        translate(const glm::vec3& _offset);
    virtual void        translate(float x, float y, float z) { translate(glm::vec3(x,y,z)); }
    virtual void        truck(float _amount) { translate(getXAxis() * _amount); };
    virtual void        boom(float _amount) { translate(getYAxis() * _amount); };
    virtual void        dolly(float _amount) { translate(getZAxis() * _amount); };

    virtual void        orbit(float _azimuth, float _elevation, float _distance, const glm::vec3& _centerPoint = glm::vec3(0.0));

    virtual void        tilt(float _degrees) { rotate(glm::angleAxis(glm::radians(_degrees), getXAxis())); };
    virtual void        pan(float _degrees) { rotate(glm::angleAxis(glm::radians(_degrees), getYAxis())); };
    virtual void        roll(float _degrees) { rotate(angleAxis(glm::radians(_degrees), getZAxis())); };

    virtual void        rotate(const glm::quat& _q);
    virtual void        rotate(float x, float y, float z, float w) { rotate(glm::quat(x,y,z,w)); }

    virtual void        rotateAround(const glm::quat& _q, const glm::vec3& _point);
    virtual void        lookAt(const glm::vec3& _lookAtPosition, glm::vec3 _upVector = glm::vec3(0.0, 1.0, 0.0));
    
    virtual void        apply(const glm::mat4& _m);
    virtual void        apply(  float x1, float y1, float z1, float w1,
                                float x2, float y2, float z2, float w2,
                                float x3, float y3, float z3, float w3,
                                float x4, float y4, float z4, float w4) {
                            apply(glm::mat4(x1, y1, z1, w1, x2, y2, z2, w2, x3, y3, z3, w3, x4, y4, z4, w4));
                        }

    virtual void        reset();

    std::vector<Node*>  childs;

    bool                bChange;

protected:
    virtual void        createMatrix();
    virtual void        updateAxis();

    virtual void        onPositionChanged() {};
    virtual void        onOrientationChanged() {};
    virtual void        onScaleChanged() {};

    glm::mat4           m_transformMatrix;
    glm::vec3           m_axis[3];

    glm::vec3           m_position;
    glm::quat           m_orientation;
    glm::vec3           m_scale;

    // NodeType            m_type;

    friend class        Label;
};

}