#pragma once

#include <vector>

#include "../types/mesh.h"
#include "../types/image.h"
#include "../types/boundingBox.h"

namespace vera {

Mesh lineMesh(const glm::vec3 &_a, const glm::vec3 &_b);
Mesh lineMesh(std::vector<glm::vec2> _points, float _width = 1.0f);
Mesh lineMesh(std::vector<glm::vec3> _points);
Mesh lineMesh(std::vector<glm::vec3> _points, float _width);

Mesh crossMesh(const glm::vec3 &_pos, float _width);

Mesh rectMesh(float _x, float _y, float _w, float _h);

Mesh axisMesh(float _size, float _y = 0.0);

Mesh gridMesh(float _size, int _segments, float _y = 0.0);
Mesh gridMesh(float _width, float _height, int _columns, int _rows, float _y = 0.0);

Mesh floorMesh(float _area, int _subD, float _y = 0.0);
Mesh planeMesh(float _width, float _height, int _columns, int _rows, DrawMode _drawMode = TRIANGLES);

Mesh boxMesh( float _width, float _height, float _depth, int _resX = 1, int _resY = 1, int _resZ = 1);

Mesh cubeMesh(float _size = 1.0f);
Mesh cubeMesh(float _size, int _resolution );

Mesh cubeCornersMesh(const BoundingBox& _bbox, float _size = 1.0f);
Mesh cubeCornersMesh(const std::vector<glm::vec3> &_pts, float _size = 1.0);
Mesh cubeCornersMesh(const glm::vec3 &_min_v, const glm::vec3 &_max_v, float _size = 1.0f);

Mesh sphereMesh(int _resolution = 24, float _radius = 1.0f, DrawMode _drawMode = TRIANGLES);
Mesh sphereHalfMesh(int _resolution = 24, float _radius = 1.0f);

Mesh icosphereMesh( float _radius, size_t _iterations );
Mesh cylinderMesh( float _radius, float _height, int _radiusSegments, int _heightSegments, int _numCapSegments, bool _bCapped, DrawMode _drawMode = TRIANGLES);
Mesh coneMesh( float _radius, float _height, int _radiusSegments, int _heightSegments, int _capSegments, DrawMode _drawMode = TRIANGLES);

Mesh toTerrain( const Image& _image,
                const float _zScale,
                const float _maxError = 0.001f, 
                const float _baseHeight = 0.0f,
                const int _maxTriangles = 0, 
                const int _maxPoints = 0 );
}
