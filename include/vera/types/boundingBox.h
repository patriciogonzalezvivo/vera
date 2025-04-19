#pragma once

#include "glm/glm.hpp"

// #include "line.h"
#include "triangle.h"

namespace vera {

class BoundingBox {
public:

    glm::vec3 min;
    glm::vec3 max;
    
    BoundingBox(): min(std::numeric_limits<float>::max()), max(std::numeric_limits<float>::min()) {}
    BoundingBox(const glm::vec2 & _center) { set(_center); }
    BoundingBox(const glm::vec3 & _center) { set(_center); }
    BoundingBox(const glm::vec4 & _b) { set(_b); }
    
    void        set(const glm::vec2& _center) { set(glm::vec3(_center, 0.0f)); }
    void        set(const glm::vec3& _center) { min = _center; max = _center; }
    void        set(const glm::vec4& _b) { min.x = _b.x; min.y = _b.y; max.x = _b.z; max.y = _b.w; }
    void        operator = (const glm::vec4& _b) { set(_b); }

    float       getWidth() const { return fabs(max.x - min.x); }
    float       getHeight() const { return fabs(max.y - min.y); }
    float       getDepth() const { return fabs(max.z - min.z); }
    
    glm::vec3   getCenter() const { return min + (max - min) * 0.5f; }
    glm::vec3   getDiagonal() const { return max - min; }
    glm::vec4   get2DBoundingBox() const { return glm::vec4(min.x, min.y, max.x, max.y); }
    float       getArea() const { glm::vec3 e = getDiagonal(); return (e.x * e.y + e.y * e.z + e.z * e.x); };// * 2.0f; }

    float       getClosestX (const float &_x) const { return  glm::max(min.x, glm::min(_x, max.x)); }
    float       getClosestY (const float &_y) const { return  glm::max(min.y, glm::min(_y, max.y)); }
    float       getClosestZ (const float &_z) const { return  glm::max(min.z, glm::min(_z, max.z)); }
    float       getClosestOn (const float &_v, const size_t _axis) const { return  glm::max(min[_axis], glm::min(_v, max[_axis])); }

    glm::vec2   getClosestPoint(const float& _x, const float& _y) const { return glm::vec2(getClosestX(_x), getClosestY(_y)); }
    glm::vec2   getClosestPoint(const glm::vec2& _point) const { return glm::vec2(getClosestX(_point.x), getClosestY(_point.y)); }
    glm::vec3   getClosestPoint(const float& _x, const float& _y, const float& _z) const { return glm::vec3(getClosestX(_x), getClosestY(_y), getClosestZ(_z)); }
    glm::vec3   getClosestPoint(const glm::vec3& _point) const {  return glm::vec3(getClosestX(_point.x), getClosestY(_point.y), getClosestZ(_point.z)); }
    
    float       getDistanceToClosest(const glm::vec2& _point) const { return glm::length( _point - getClosestPoint(_point) );}
    float       getDistanceToClosest(const glm::vec3& _point) const { return glm::length( _point - getClosestPoint(_point) );}

    bool        containsX(const float& _x) const { return _x >= min.x && _x <= max.x; }
    bool        containsY(const float& _y) const { return _y >= min.y && _y <= max.y; }
    bool        containsZ(const float& _z) const { return _z >= min.z && _z <= max.z; }

    bool        contains(const float& _x, const float& _y) const { return containsX(_x) && containsY(_y); }
    bool        contains(const glm::vec2& _v) const { return containsX(_v.x) && containsY(_v.y); }
    bool        contains(const float& _x, const float& _y, const float& _z) const { return containsX(_x) && containsY(_y) && containsZ(_z); }
    bool        contains(const glm::vec3& _v) const { return containsX(_v.x) && containsY(_v.y) && containsZ(_v.z); }

    bool        intersects(const BoundingBox& _b) const {  return (min.x <= _b.max.x && max.x >= _b.min.x) && (min.y <= _b.max.y && max.y >= _b.min.y) && (min.z <= _b.max.z && max.z >= _b.min.z); }
    // bool        intersects(const BoundingBox& _b) const { return (min.x < _b.max.x) && (max.x > _b.min.x) && (min.y < _b.max.y) && (max.y > _b.min.y) && (min.z < _b.max.z) && (max.z > _b.min.z) && *this != _b; }
    // bool        operator==(const BoundingBox& _b) const { return (max.x == _b.max.x && max.y == _b.max.y && max.z == _b.max.z && min.x == _b.min.x && min.y == _b.min.y && min.z == _b.min.z); }
    // bool        operator!=(const BoundingBox& _b) const { return (max.x != _b.max.x) || (max.y != _b.max.y) || (max.z != _b.max.z) || (min.x != _b.min.x) || (min.y != _b.min.y) || (min.z != _b.min.z); }

    bool        intersects(const glm::vec3 _p, float _r2) const {
        float dmin = 0.0f;
        for( size_t i = 0; i < 3; i++ ) {
            if( _p[i] < min[i] ) dmin += std::sqrt( _p[i] - min[i] ); 
            else if( _p[i] > max[i] ) dmin += std::sqrt( _p[i] - max[i] );     
        }
        
        if( dmin <= _r2 ) return true;
        return false;
    }

    void        expand(float _value) {
        min -= _value;
        max += _value;
    }

    void        expand(float _x, float _y) {
        min.x = std::min(min.x, _x);
        max.x = std::max(max.x, _x);
        
        min.y = std::min(min.y, _y);
        max.y = std::max(max.y, _y);
    }

    void        expand(float _x, float _y, float _z) {
        expand(_x, _y);
        min.z = std::min(min.z, _z);
        max.z = std::max(max.z, _z);
    }

    void        expand(const glm::vec2& _v) { expand(_v.x, _v.y); }
    void        expand(const glm::vec3& _v) { expand(_v.x, _v.y, _v.z); }
    void        expand(const Triangle& _t) { expand(_t[0]); expand(_t[1]); expand(_t[2]); }
    void        expand(const BoundingBox& _b) { expand(_b.min); expand(_b.max); }

    void        square() {
        glm::vec3   center  = getCenter();
        glm::vec3   diag    = getDiagonal() * 0.5f;
        float       mmax    = glm::max( abs(diag.x), glm::max( abs(diag.y), abs(diag.z) ) );
        max                 = center + mmax;
        min                 = center - mmax;
    }

    void        clean() { 
        min = glm::vec3(std::numeric_limits<float>::max()); 
        max = glm::vec3(std::numeric_limits<float>::min()); 
    }
    
};

}