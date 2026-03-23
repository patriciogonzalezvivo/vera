#pragma once

#include "vera/types/ray.h"
#include "vera/types/line.h"
#include "vera/types/plane.h"
#include "vera/types/triangle.h"
#include "vera/types/boundingBox.h"

#include <string>

// Geometric intersection and distance calculations.
// Based on http://paulbourke.net/geometry/pointlineplane/
// and https://github.com/robandrose/ofxIntersection

namespace vera {

// =============================================================================
// 2D INTERSECTION TESTS
// =============================================================================

/// Test if a point is inside a 2D polygon
/// @param _points Polygon vertices
/// @param _v Point to test
/// @return True if point is inside polygon
bool inside(const std::vector<glm::vec2> _points, const glm::vec2 _v);

/// Find intersection point of two 2D line segments
/// @param _line1Start First line start point
/// @param _line1End First line end point
/// @param _line2Start Second line start point
/// @param _line2End Second line end point
/// @param _intersection Output intersection point
/// @return True if lines intersect
bool intersection(  const glm::vec2 &_line1Start, const glm::vec2 &_line1End,
                    const glm::vec2 &_line2Start, const glm::vec2 &_line2End,
                    glm::vec2 &_intersection );

/// Find intersection of 2D line segment with bounding box
/// @param _lineStart Line start point
/// @param _lineEnd Line end point
/// @param _bbox Bounding box
/// @param _intersection Output intersection point
/// @return True if line intersects box
bool intersection( const glm::vec2& _lineStart, const glm::vec2& _lineEnd, 
                    const BoundingBox& _bbox,
                    glm::vec2 &_intersection );

/// Find all intersection points of polygon with bounding box
/// @param _polygon Polygon vertices
/// @param _bbox Bounding box
/// @param _intersections Output intersection points
/// @return True if any intersections found
bool intersection( const std::vector<glm::vec2>& _polygon, 
                    const BoundingBox& _bbox, 
                    std::vector<glm::vec2>& _intersections );

// =============================================================================
// INTERSECTION DATA STRUCTURE
// =============================================================================

/// Result data from 3D intersection tests
struct IntersectionData {
    IntersectionData() = default;
    glm::vec3   position;       ///< Intersection point in world space
    glm::vec3   direction;      ///< Direction vector at intersection
    float       distance = 0.0f;///< Distance from ray origin to intersection
    bool        hit = false;    ///< True if intersection occurred
};

// =============================================================================
// RAY-PLANE INTERSECTIONS
// =============================================================================

/// Calculate signed distance from point to plane
/// @param _point Point to test
/// @param _plane Plane
/// @return Signed distance (positive = above plane, negative = below)
float               distance(const glm::vec3& _point, const Plane& _plane);

/// Find intersection of ray with plane
/// @param _ray Ray
/// @param _plane Plane
/// @return Intersection data
IntersectionData    intersection(const Ray& _ray, const Plane& _plane);

// =============================================================================
// RAY-BOUNDING BOX INTERSECTIONS
// =============================================================================

/// Find intersection of ray with bounding box
/// @param _ray Ray
/// @param _bbox Bounding box
/// @return Intersection data
IntersectionData    intersection(const Ray& _ray, const BoundingBox& _bbox);

/// Test ray-box intersection and get entry/exit distances
/// @param _ray Ray
/// @param _bbox Bounding box
/// @param _tmin Output entry distance along ray
/// @param _tmax Output exit distance along ray
/// @return True if ray intersects box
bool                intersection(const Ray& _ray, const BoundingBox& _bbox, float& _tmin, float& _tmax);

// =============================================================================
// RAY-TRIANGLE INTERSECTIONS
// =============================================================================

/// Calculate distance from point to triangle and find closest point
/// @param _point Point to test
/// @param _tri Triangle
/// @param _closesPoint Output closest point on triangle
/// @return Distance to triangle
float               distance(const glm::vec3& _point, const Triangle& _tri, glm::vec3& _closesPoint);

/// Find intersection of ray with triangle
/// @param _ray Ray
/// @param _triangle Triangle
/// @return Intersection data
IntersectionData    intersection(const Ray& _ray, const Triangle& _triangle);

/// Test ray-triangle intersection using Möller-Trumbore algorithm
/// @param _ray Ray
/// @param _triangle Triangle
/// @param _t Output distance along ray to intersection
/// @param _u Output barycentric U coordinate
/// @param _v Output barycentric V coordinate
/// @return True if ray intersects triangle
bool                intersection(const Ray& _ray, const Triangle& _triangle, float& _t, float& _u, float& _v);

// =============================================================================
// LINE INTERSECTIONS
// =============================================================================

/// Calculate distance from point to line and find closest point
/// @param _point Point to test
/// @param _line Line
/// @param _closes_point Output closest point on line
/// @return Distance to line
float               distance(const glm::vec3& _point, const Line& _line, glm::vec3& _closes_point);

/// Find closest point on line to given point
/// @param _point Point to test
/// @param _line Line
/// @return Intersection data with closest point
IntersectionData    intersection(const glm::vec3& _point, const Line& _line);

/// Find intersection of line with plane
/// @param _line Line
/// @param _plane Plane
/// @return Intersection data
IntersectionData    intersection(const Line& _line, const Plane& _plane);

/// Find closest points between two 3D lines
/// @param _line1 First line
/// @param _line2 Second line
/// @return Intersection data
IntersectionData    intersection(const Line& _line1, const Line& _line2);

/// Test if two lines intersect and get intersection point
/// @param _line1 First line
/// @param _line2 Second line
/// @param _p Output intersection point
/// @return True if lines intersect (or are very close)
bool                intersection(const Line& _line1, const Line& _line2, glm::vec3& _p );

/// Test if two lines intersect within tolerance
/// @param _line1 First line
/// @param _line2 Second line
/// @param _p Output intersection point
/// @param _threshold Maximum distance to consider as intersection
/// @return True if lines are within threshold distance
bool                intersection(const Line& _line1, const Line& _line2, glm::vec3& _p, float _threshold );

// =============================================================================
// PLANE-PLANE AND PLANE-TRIANGLE INTERSECTIONS
// =============================================================================

/// Find intersection line of two planes
/// @param _plane1 First plane
/// @param _plane2 Second plane
/// @return Intersection data with line direction
IntersectionData    intersection(const Plane& _plane1, const Plane& _plane2);

/// Find intersection of plane with triangle
/// @param _plane Plane
/// @param _triangle Triangle
/// @return Intersection data
IntersectionData    intersection(const Plane& _plane, const Triangle& _triangle);

}
