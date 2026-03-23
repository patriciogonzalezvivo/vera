#pragma once

#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "../types/mesh.h"
#include "../types/line.h"
#include "../types/triangle.h"
#include "../types/boundingBox.h"

// Geometry utility functions for 2D and 3D operations.
// 2D operations work on vectors of glm::vec2; 3D on glm::vec3 or Mesh.
// Uses O(n log n) algorithms for convex hull (Graham/Andrew monotone-chain scan).
// Douglas-Peucker algorithm for polyline simplification.

namespace vera {

// =============================================================================
// 2D GEOMETRY OPERATIONS
// =============================================================================

/// Simplify a 2D polyline using Douglas-Peucker algorithm (in-place)
/// @param _pts Points to simplify
/// @param _tolerance Maximum distance from simplified line (default: 0.3)
void                    simplify(std::vector<glm::vec2> &_pts, float _tolerance=0.3f);

/// Calculate area of 2D polygon
/// @param _pts Polygon vertices
/// @return Signed area (positive if counterclockwise)
float                   getArea(const std::vector<glm::vec2>& _pts);

/// Calculate centroid of 2D polygon
/// @param _pts Polygon vertices  
/// @return Centroid point
glm::vec2               getCentroid(const std::vector<glm::vec2>& _pts);

/// Get simplified 2D polyline using Douglas-Peucker algorithm
/// @param _pts Points to simplify
/// @param _tolerance Maximum distance from simplified line (default: 0.3)
/// @return Simplified point list
std::vector<glm::vec2>  getSimplify(const std::vector<glm::vec2> &_pts, float _tolerance=0.3f);

/// Compute convex hull of 2D points
/// @param _pts Input points
/// @return Vertices of convex hull in counterclockwise order
std::vector<glm::vec2>  getConvexHull(const std::vector<glm::vec2> &_pts);

/// Compute bounding box of 2D points
/// @param _points Input points
/// @return Axis-aligned bounding box
BoundingBox             getBoundingBox(const std::vector<glm::vec2>& _points);

// =============================================================================
// 3D GEOMETRY OPERATIONS
// =============================================================================

/// Calculate centroid of 3D points
/// @param _pts Input points
/// @return Centroid point
glm::vec3   getCentroid(const std::vector<glm::vec3>& _pts);

/// Compute bounding box of a mesh
/// @param _mesh Input mesh
/// @return Axis-aligned bounding box
BoundingBox getBoundingBox(const Mesh& _mesh);

/// Compute bounding box of 3D points
/// @param _points Input points
/// @return Axis-aligned bounding box
BoundingBox getBoundingBox(const std::vector<glm::vec3>& _points);

/// Compute bounding box of line segments
/// @param _lines Input lines
/// @return Axis-aligned bounding box
BoundingBox getBoundingBox(const std::vector<Line>& _lines);

/// Compute bounding box of triangles
/// @param _triangles Input triangles
/// @return Axis-aligned bounding box
BoundingBox getBoundingBox(const std::vector<Triangle>& _triangles);

/// Calculate normal vector for a triangle
/// @param _v0 First vertex
/// @param _v1 Second vertex
/// @param _v2 Third vertex
/// @param _N Output normal vector (normalized)
void        calcNormal(const glm::vec3& _v0, const glm::vec3& _v1, const glm::vec3& _v2, glm::vec3& _N);

// =============================================================================
// 3D POINT TRANSFORMATIONS
// =============================================================================

/// Transform points by quaternion rotation
/// @param _points Points to transform (modified in-place)
/// @param _mat Rotation quaternion
void        transform(std::vector<glm::vec3>& _points, const glm::quat& _mat);

/// Transform points by 3x3 matrix
/// @param _points Points to transform (modified in-place)
/// @param _mat Transformation matrix
void        transform(std::vector<glm::vec3>& _points, const glm::mat3& _mat);

/// Transform points by 4x4 matrix
/// @param _points Points to transform (modified in-place)
/// @param _mat Transformation matrix
void        transform(std::vector<glm::vec3>& _points, const glm::mat4& _mat);

/// Scale points uniformly
/// @param _points Points to scale (modified in-place)
/// @param _v Scale factor
void        scale(std::vector<glm::vec3>& _points, float _v);

/// Scale points along X axis
/// @param _points Points to scale (modified in-place)
/// @param _x X scale factor
void        scaleX(std::vector<glm::vec3>& _points, float _x);

/// Scale points along Y axis
/// @param _points Points to scale (modified in-place)
/// @param _y Y scale factor
void        scaleY(std::vector<glm::vec3>& _points, float _y);

/// Scale points along Z axis
/// @param _points Points to scale (modified in-place)
/// @param _z Z scale factor
void        scaleZ(std::vector<glm::vec3>& _points, float _z);

/// Scale points by vector
/// @param _points Points to scale (modified in-place)
/// @param _v Scale vector (x, y, z)
void        scale(std::vector<glm::vec3>& _points, const glm::vec3& _v);

/// Scale points non-uniformly
/// @param _points Points to scale (modified in-place)
/// @param _x X scale factor
/// @param _y Y scale factor
/// @param _z Z scale factor (default: 1.0)
void        scale(std::vector<glm::vec3>& _points, float _x, float _y, float _z = 1.0f);

/// Translate points along X axis
/// @param _points Points to translate (modified in-place)
/// @param _x X translation
void        translateX(std::vector<glm::vec3>& _points, float _x);

/// Translate points along Y axis
/// @param _points Points to translate (modified in-place)
/// @param _y Y translation
void        translateY(std::vector<glm::vec3>& _points, float _y);

/// Translate points along Z axis
/// @param _points Points to translate (modified in-place)
/// @param _z Z translation
void        translateZ(std::vector<glm::vec3>& _points, float _z);

/// Translate points by vector
/// @param _points Points to translate (modified in-place)
/// @param _v Translation vector
void        translate(std::vector<glm::vec3>& _points, const glm::vec3& _v);

/// Translate points
/// @param _points Points to translate (modified in-place)
/// @param _x X translation
/// @param _y Y translation
/// @param _z Z translation (default: 0.0)
void        translate(std::vector<glm::vec3>& _points, float _x, float _y, float _z = 0.0f);

/// Rotate points around X axis
/// @param _points Points to rotate (modified in-place)
/// @param _rad Rotation angle in radians
void        rotateX(std::vector<glm::vec3>& _points, float _rad);

/// Rotate points around Y axis
/// @param _points Points to rotate (modified in-place)
/// @param _rad Rotation angle in radians
void        rotateY(std::vector<glm::vec3>& _points, float _rad);

/// Rotate points around Z axis
/// @param _points Points to rotate (modified in-place)
/// @param _rad Rotation angle in radians
void        rotateZ(std::vector<glm::vec3>& _points, float _rad);

/// Rotate points around arbitrary axis
/// @param _points Points to rotate (modified in-place)
/// @param _rad Rotation angle in radians
/// @param _axis Rotation axis (normalized)
void        rotate(std::vector<glm::vec3>& _points, float _rad, const glm::vec3& _axis );

/// Rotate points around arbitrary axis
/// @param _points Points to rotate (modified in-place)
/// @param _rad Rotation angle in radians
/// @param _x X component of rotation axis
/// @param _y Y component of rotation axis
/// @param _z Z component of rotation axis
void        rotate(std::vector<glm::vec3>& _points, float _rad, float _x, float _y, float _z );

/// Center points around origin (translate centroid to origin)
/// @param _points Points to center (modified in-place)
void        center(std::vector<glm::vec3>& _points);

// =============================================================================
// MESH TRANSFORMATIONS (inline wrappers for point operations)
// =============================================================================

/// Transform mesh by quaternion rotation
inline void transform(Mesh& _mesh, const glm::quat& _mat) { transform(_mesh.m_vertices, _mat); }

/// Transform mesh by 3x3 matrix
inline void transform(Mesh& _mesh, const glm::mat3& _mat) { transform(_mesh.m_vertices, _mat); }

/// Transform mesh by 4x4 matrix
inline void transform(Mesh& _mesh, const glm::mat4& _mat) { transform(_mesh.m_vertices, _mat); }

/// Scale mesh uniformly
inline void scale(Mesh& _mesh, float _v) { scale(_mesh.m_vertices, _v); };

/// Scale mesh along X axis
inline void scaleX(Mesh& _mesh, float _x) { scaleX(_mesh.m_vertices, _x); };

/// Scale mesh along Y axis
inline void scaleY(Mesh& _mesh, float _y) { scaleY(_mesh.m_vertices, _y); };

/// Scale mesh along Z axis
inline void scaleZ(Mesh& _mesh, float _z) { scaleZ(_mesh.m_vertices, _z); };

/// Scale mesh by vector
inline void scale(Mesh& _mesh, const glm::vec3& _v) { scale(_mesh.m_vertices, _v); };

/// Scale mesh non-uniformly
inline void scale(Mesh& _mesh, float _x, float _y, float _z = 1.0f) { scale(_mesh.m_vertices, _x, _y, _z); };

/// Translate mesh along X axis
inline void translateX(Mesh& _mesh, float _x) { translateX(_mesh.m_vertices, _x); };

/// Translate mesh along Y axis
inline void translateY(Mesh& _mesh, float _y) { translateY(_mesh.m_vertices, _y); };

/// Translate mesh along Z axis
inline void translateZ(Mesh& _mesh, float _z) { translateZ(_mesh.m_vertices, _z); };

/// Translate mesh by vector
inline void translate(Mesh& _mesh, const glm::vec3& _v) { translate(_mesh.m_vertices, _v); };

/// Translate mesh
inline void translate(Mesh& _mesh, float _x, float _y, float _z = 0.0f) { translate(_mesh.m_vertices, _x, _y, _z);};

/// Rotate mesh around X axis
/// @param _mesh Mesh to rotate (modified in-place)
/// @param _rad Rotation angle in radians
void        rotateX(Mesh& _mesh, float _rad);

/// Rotate mesh around Y axis
/// @param _mesh Mesh to rotate (modified in-place)
/// @param _rad Rotation angle in radians
void        rotateY(Mesh& _mesh, float _rad);

/// Rotate mesh around Z axis
/// @param _mesh Mesh to rotate (modified in-place)
/// @param _rad Rotation angle in radians
void        rotateZ(Mesh& _mesh, float _rad);

/// Rotate mesh around arbitrary axis
/// @param _mesh Mesh to rotate (modified in-place)
/// @param _rad Rotation angle in radians
/// @param _axis Rotation axis (normalized)
void        rotate(Mesh& _mesh, float _rad, const glm::vec3& _axis );

/// Rotate mesh around arbitrary axis
/// @param _mesh Mesh to rotate (modified in-place)
/// @param _rad Rotation angle in radians
/// @param _x X component of rotation axis
/// @param _y Y component of rotation axis
/// @param _z Z component of rotation axis
void        rotate(Mesh& _mesh, float _rad, float _x, float _y, float _z );

/// Center mesh around origin
inline void center(Mesh& _mesh) { center(_mesh.m_vertices); };

}
