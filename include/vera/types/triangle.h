#pragma once

#include <vector>

#include "glm/glm.hpp"

// #include "vera/types/material.h"

namespace vera {

class Triangle {
public :
    
    Triangle();
    Triangle(const glm::vec3 &_p0, const glm::vec3 &_p1, const glm::vec3 &_p2);
    virtual ~Triangle();

    void set(const glm::vec3 &_p0, const glm::vec3 &_p1, const glm::vec3 &_p2);
    void setColors(const glm::vec4 &_p0, const glm::vec4 &_p1, const glm::vec4 &_p2);
    void setNormals(const glm::vec3 &_p0, const glm::vec3 &_p1, const glm::vec3 &_p2);
    void setTexCoords(const glm::vec2 &_p0, const glm::vec2 &_p1, const glm::vec2 &_p2);
    void setTangents(const glm::vec4 &_p0, const glm::vec4 &_p1, const glm::vec4 &_p2);

    bool containsPoint(const glm::vec3 &_p);
    
    // Getters:
    const glm::vec3&    operator[](size_t _index) const { return m_vertices[_index]; }
    void                setVertex(size_t _index, const glm::vec3& _vertex);
    const glm::vec3&    getVertex(size_t _index) const { return m_vertices[_index]; }
    glm::vec3           getVertex(const glm::vec3& _barycenterCoord ) const;

    glm::vec3           getCentroid() const { return (m_vertices[0] + m_vertices[1] + m_vertices[2]) * 0.3333333333333f; }
    glm::vec3           getMin() const { return glm::vec3(  std::min(m_vertices[0].x, std::min(m_vertices[1].x, m_vertices[2].x)),
                                                            std::min(m_vertices[0].y, std::min(m_vertices[1].y, m_vertices[2].y)),
                                                            std::min(m_vertices[0].z, std::min(m_vertices[1].z, m_vertices[2].z)) ); }
    glm::vec3           getMax() const { return glm::vec3(  std::max(m_vertices[0].x, std::max(m_vertices[1].x, m_vertices[2].x)),
                                                            std::max(m_vertices[0].y, std::max(m_vertices[1].y, m_vertices[2].y)),
                                                            std::max(m_vertices[0].z, std::max(m_vertices[1].z, m_vertices[2].z)) ); }
    glm::vec3           getBarycentric() const;
    glm::vec3           getBarycentricOf( const glm::vec3& _p ) const;
    static glm::vec3    getBarycentric(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);
    
    bool                haveColors() const { return !m_colors.empty(); }
    void                setColor(const glm::vec4 &_color);
    void                setColor(float _r, float _g, float _b, float _a = 1.0f);
    void                setColor(size_t _index, const glm::vec4& _color);
    const glm::vec4&    getColor(size_t _index) const { return m_colors[_index]; }
    glm::vec4           getColor(const glm::vec3& _barycenterCoord ) const;

    bool                haveNormals() const { return !m_normals.empty(); }
    void                setNormal(size_t _index, const glm::vec3& _normal);
    const glm::vec3&    getNormal() const { return m_normal; }
    const glm::vec3&    getNormal(size_t _index) const;
    glm::vec3           getNormal(const glm::vec3& _barycenterCoord ) const;

    bool                haveTexCoords() const { return !m_texCoords.empty(); }
    void                setTexCoord(size_t _index, const glm::vec2& _texcoord);
    const glm::vec2&    getTexCoord(size_t _index) const { return m_texCoords[_index]; }
    glm::vec2           getTexCoord(const glm::vec3& _barycenterCoord ) const;

    bool                haveTangents() const { return !m_tangents.empty(); }
    void                setTangent(size_t _index, const glm::vec4& _tangent);
    const glm::vec4&    getTangent(size_t _index) const { return m_tangents[_index]; }
    glm::vec4           getTangent(const glm::vec3& _barycenterCoord ) const;
    
    // MaterialConstPtr    material = nullptr;
    // size_t              closestCoorner(const glm::vec3& _p) const;
    glm::vec3           closest(const glm::vec3& _p) const;
    float               signedDistance(const glm::vec3& _p) const;
    float               unsignedDistance(const glm::vec3& _p) const;

    static bool         compare(const Triangle& _a, const Triangle& _b, size_t _axis) {
        return _a.getCentroid()[_axis] < _b.getCentroid()[_axis];
    }
    static bool         compareX (const Triangle& a, const Triangle& b) { return compare(a, b, 0); }
    static bool         compareY (const Triangle& a, const Triangle& b) { return compare(a, b, 1); }
    static bool         compareZ (const Triangle& a, const Triangle& b) { return compare(a, b, 2); }

private:
    glm::vec3               m_vertices[3];
    glm::vec3               m_normal;
    float                   m_area;

    std::vector<glm::vec4>  m_colors;
    std::vector<glm::vec3>  m_normals;
    std::vector<glm::vec2>  m_texCoords;
    std::vector<glm::vec4>  m_tangents;
    
};


}