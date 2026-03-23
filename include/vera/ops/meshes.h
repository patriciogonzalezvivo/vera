#pragma once

#include <vector>

#include "../types/mesh.h"
#include "../types/image.h"
#include "../types/boundingBox.h"

namespace vera {

// =============================================================================
// LINE MESHES
// =============================================================================

/// Create a line mesh between two 3D points
/// @param _a Start point
/// @param _b End point
/// @return Line mesh
Mesh lineMesh(const glm::vec3 &_a, const glm::vec3 &_b);

/// Create a thick line mesh from 2D points
/// @param _points Line vertices
/// @param _width Line width (default: 1.0)
/// @return Line mesh with width
Mesh lineMesh(std::vector<glm::vec2> _points, float _width = 1.0f);

/// Create a line mesh from 3D points
/// @param _points Line vertices
/// @return Polyline mesh
Mesh lineMesh(std::vector<glm::vec3> _points);

/// Create a thick line mesh from 3D points
/// @param _points Line vertices
/// @param _width Line width
/// @return Line mesh with width
Mesh lineMesh(std::vector<glm::vec3> _points, float _width);

// =============================================================================
// BASIC SHAPE MESHES
// =============================================================================

/// Create a 3D cross (plus sign) mesh at a position
/// @param _pos Center position
/// @param _width Cross arm width
/// @return Cross mesh
Mesh crossMesh(const glm::vec3 &_pos, float _width);

/// Create a 2D rectangle mesh
/// @param _x X position
/// @param _y Y position
/// @param _w Width
/// @param _h Height
/// @return Rectangle mesh
Mesh rectMesh(float _x, float _y, float _w, float _h);

// =============================================================================
// GRID AND PLANE MESHES
// =============================================================================

/// Create a 3-axis visualization mesh
/// @param _size Axis length
/// @param _y Y position (default: 0.0)
/// @return Axis mesh (X=red, Y=green, Z=blue direction)
Mesh axisMesh(float _size, float _y = 0.0);

/// Create a square grid mesh
/// @param _size Grid size (width and height)
/// @param _segments Number of grid divisions per side
/// @param _y Y position (default: 0.0)
/// @return Grid mesh
Mesh gridMesh(float _size, int _segments, float _y = 0.0);

/// Create a rectangular grid mesh
/// @param _width Grid width
/// @param _height Grid height
/// @param _columns Number of horizontal divisions
/// @param _rows Number of vertical divisions
/// @param _y Y position (default: 0.0)
/// @return Grid mesh
Mesh gridMesh(float _width, float _height, int _columns, int _rows, float _y = 0.0);

/// Create a subdivided floor mesh
/// @param _area Floor area (width and height)
/// @param _subD Subdivision level
/// @param _y Y position (default: 0.0)
/// @return Floor mesh
Mesh floorMesh(float _area, int _subD, float _y = 0.0);

/// Create a plane mesh with specified topology
/// @param _width Plane width
/// @param _height Plane height
/// @param _columns Number of horizontal divisions
/// @param _rows Number of vertical divisions
/// @param _drawMode Rendering mode (default: TRIANGLES)
/// @return Plane mesh
Mesh planeMesh(float _width, float _height, int _columns, int _rows, DrawMode _drawMode = TRIANGLES);

// =============================================================================
// BOX AND CUBE MESHES
// =============================================================================

/// Create a rectangular box mesh with subdivisions
/// @param _width Box width (X dimension)
/// @param _height Box height (Y dimension)
/// @param _depth Box depth (Z dimension)
/// @param _resX X resolution (default: 1)
/// @param _resY Y resolution (default: 1)
/// @param _resZ Z resolution (default: 1)
/// @return Box mesh
Mesh boxMesh( float _width, float _height, float _depth, int _resX = 1, int _resY = 1, int _resZ = 1);

/// Create a simple cube mesh
/// @param _size Cube size (default: 1.0)
/// @return Cube mesh
Mesh cubeMesh(float _size = 1.0f);

/// Create a subdivided cube mesh
/// @param _size Cube size
/// @param _resolution Number of subdivisions per face
/// @return Subdivided cube mesh
Mesh cubeMesh(float _size, int _resolution );

/// Create corner markers for a bounding box
/// @param _bbox Bounding box
/// @param _size Corner marker size (default: 1.0)
/// @return Mesh with small cubes at each corner
Mesh cubeCornersMesh(const BoundingBox& _bbox, float _size = 1.0f);

/// Create corner markers for a point cloud
/// @param _pts Point cloud
/// @param _size Corner marker size (default: 1.0)
/// @return Mesh with small cubes at bounding box corners
Mesh cubeCornersMesh(const std::vector<glm::vec3> &_pts, float _size = 1.0);

/// Create corner markers for a bounding box defined by min/max
/// @param _min_v Minimum corner
/// @param _max_v Maximum corner
/// @param _size Corner marker size (default: 1.0)
/// @return Mesh with small cubes at each corner
Mesh cubeCornersMesh(const glm::vec3 &_min_v, const glm::vec3 &_max_v, float _size = 1.0f);

// =============================================================================
// SPHERE AND CURVED SURFACE MESHES
// =============================================================================

/// Create a UV sphere mesh
/// @param _resolution Number of longitude/latitude divisions (default: 24)
/// @param _radius Sphere radius (default: 1.0)
/// @param _drawMode Rendering mode (default: TRIANGLES)
/// @return Sphere mesh
Mesh sphereMesh(int _resolution = 24, float _radius = 1.0f, DrawMode _drawMode = TRIANGLES);

/// Create a hemisphere mesh
/// @param _resolution Number of longitude/latitude divisions (default: 24)
/// @param _radius Hemisphere radius (default: 1.0)
/// @return Hemisphere mesh (upper half of sphere)
Mesh sphereHalfMesh(int _resolution = 24, float _radius = 1.0f);

/// Create an icosphere mesh (geodesic sphere)
/// @param _radius Sphere radius
/// @param _iterations Number of subdivision iterations
/// @return Icosphere mesh
Mesh icosphereMesh( float _radius, size_t _iterations );

/// Create a cylinder mesh
/// @param _radius Cylinder radius
/// @param _height Cylinder height
/// @param _radiusSegments Number of circular segments
/// @param _heightSegments Number of height divisions
/// @param _numCapSegments Number of radial cap divisions
/// @param _bCapped Add end caps if true
/// @param _drawMode Rendering mode (default: TRIANGLES)
/// @return Cylinder mesh
Mesh cylinderMesh( float _radius, float _height, int _radiusSegments, int _heightSegments, int _numCapSegments, bool _bCapped, DrawMode _drawMode = TRIANGLES);

/// Create a cone mesh
/// @param _radius Base radius
/// @param _height Cone height
/// @param _radiusSegments Number of circular segments
/// @param _heightSegments Number of height divisions
/// @param _capSegments Number of radial cap divisions
/// @param _drawMode Rendering mode (default: TRIANGLES)
/// @return Cone mesh
Mesh coneMesh( float _radius, float _height, int _radiusSegments, int _heightSegments, int _capSegments, DrawMode _drawMode = TRIANGLES);

// =============================================================================
// TERRAIN GENERATION
// =============================================================================

/// Generate terrain mesh from heightmap image
/// @param _image Heightmap image (grayscale)
/// @param _zScale Vertical scale factor for height
/// @param _maxError Maximum allowed error for mesh simplification (default: 0.001)
/// @param _baseHeight Base elevation (default: 0.0)
/// @param _maxTriangles Maximum triangle count limit (0 = no limit)
/// @param _maxPoints Maximum vertex count limit (0 = no limit)
/// @return Optimized terrain mesh
Mesh toTerrain( const Image& _image,
                const float _zScale,
                const float _maxError = 0.001f, 
                const float _baseHeight = 0.0f,
                const int _maxTriangles = 0, 
                const int _maxPoints = 0 );
}
