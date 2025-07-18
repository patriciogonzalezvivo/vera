#pragma once

#include "vera/types/ray.h"
#include "vera/types/line.h"
#include "vera/types/plane.h"
#include "vera/types/triangle.h"
#include "vera/types/boundingBox.h"

#include <string>

// Based on
// https://github.com/robandrose/ofxIntersection
// http://paulbourke.net/geometry/pointlineplane/

namespace vera {

// 2D
//
bool inside(const std::vector<glm::vec2> _points, const glm::vec2 _v);

bool intersection(  const glm::vec2 &_line1Start, const glm::vec2 &_line1End,
                    const glm::vec2 &_line2Start, const glm::vec2 &_line2End,
                    glm::vec2 &_intersection );

bool intersection( const glm::vec2& _lineStart, const glm::vec2& _lineEnd, 
                    const BoundingBox& _bbox,
                    glm::vec2 &_intersection );

bool intersection( const std::vector<glm::vec2>& _polygon, 
                    const BoundingBox& _bbox, 
                    std::vector<glm::vec2>& _intersections );


// 3D
//
struct IntersectionData {
    IntersectionData() = default;
    glm::vec3   position;
    glm::vec3   direction;
    float       distance    = 0.0f;
    bool        hit         = false;
};


float               distance(const glm::vec3& _point, const Plane& _plane);
IntersectionData    intersection(const Ray& _ray, const Plane& _plane);

IntersectionData    intersection(const Ray& _ray, const BoundingBox& _bbox);
bool                intersection(const Ray& _ray, const BoundingBox& _bbox, float& _tmin, float& _tmax);

float               distance(const glm::vec3& _point, const Triangle& _tri, glm::vec3& _closesPoint);
IntersectionData    intersection(const Ray& _ray, const Triangle& _triangle);
bool                intersection(const Ray& _ray, const Triangle& _triangle, float& _t, float& _u, float& _v);

// Line
float               distance(const glm::vec3& _point, const Line& _line, glm::vec3& _closes_point);
IntersectionData    intersection(const glm::vec3& _point, const Line& _line);

IntersectionData    intersection(const Line& _line, const Plane& _plane);

IntersectionData    intersection(const Line& _line1, const Line& _line2);
bool                intersection(const Line& _line1, const Line& _line2, glm::vec3& _p );
bool                intersection(const Line& _line1, const Line& _line2, glm::vec3& _p, float _threshold );

// Plane
IntersectionData    intersection(const Plane& _plane1, const Plane& _plane2);
IntersectionData    intersection(const Plane& _plane, const Triangle& _triangle);

}
