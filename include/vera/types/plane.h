
#pragma once

#include "glm/glm.hpp"
#include "triangle.h"

namespace vera {

class Plane {
public:
    Plane(): origin(0.0), normal(0.0,0.0,1.0), dCoeff(0.0){ };
    
    Plane(const glm::vec3& _pos, const glm::vec3& _normal) { set(_pos, _normal); };
    Plane(const Triangle& _tri) { set(_tri); };
    
    void set(const Triangle& _tri) { set(_tri.getCentroid(), _tri.getNormal()); }
    void set(const glm::vec3& _pos, const glm::vec3& _normal){
        origin = _pos;
        setNormal(_normal);
    }
    
    void setNormal(const glm::vec3& _normal) {
        normal = glm::normalize(_normal);
        dCoeff = glm::dot(normal, origin);
    }
    glm::vec3 getNormal() const {return origin;};
    
    void setOrigin(const glm::vec3& _pos) {
        origin = _pos;
        dCoeff = glm::dot(normal, origin);
    }
    glm::vec3 getOrigin() const {return origin;};
    
    float getDCoeff() const {return dCoeff;};
    
protected:
    glm::vec3   origin;
    glm::vec3   normal;
    float       dCoeff;
};

}