#include "vera/ops/meshes.h"
#include "vera/ops/geom.h"
#include "vera/ops/math.h"

#include <iostream>
#include <algorithm>
#include <map>
#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtx/norm.hpp"

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif
#ifndef TAU
#define TAU 6.2831853071795864769252867665590
#endif

namespace vera {

Mesh lineMesh(const glm::vec3 &_a, const glm::vec3 &_b) {
    glm::vec3 linePoints[2];
    linePoints[0] = glm::vec3(_a.x,_a.y,_a.z);
    linePoints[1] = glm::vec3(_b.x,_b.y,_b.z);;

    Mesh mesh;
    mesh.addVertices(linePoints,2);
    mesh.setDrawMode(LINES);
    return mesh;
};

Mesh lineToMesh(const glm::vec3 &_a, const glm::vec3 &_dir, float _size) {
    return lineMesh(_a, _a + normalize(_dir) * _size );
}

Mesh crossMesh(const glm::vec3 &_pos, float _width) {
    glm::vec3 linePoints[4] = { glm::vec3(_pos.x,_pos.y,_pos.z),
                                glm::vec3(_pos.x,_pos.y,_pos.z),
                                glm::vec3(_pos.x,_pos.y,_pos.z),
                                glm::vec3(_pos.x,_pos.y,_pos.z) };

    linePoints[0].x -= _width;
    linePoints[1].x += _width;
    linePoints[2].y -= _width;
    linePoints[3].y += _width;

    Mesh mesh;
    mesh.setDrawMode(LINES);
    mesh.addVertices(linePoints, 4);

    // mesh.append( line(linePoints[0] , linePoints[1]) );
    // mesh.append( line(linePoints[2] , linePoints[3]) );

    return mesh;
}


// Billboard
//============================================================================
Mesh rectMesh(float _x, float _y, float _w, float _h) {
    float x = _x * 2.0f - 1.0f;
    float y = _y * 2.0f - 1.0f;
    float w = _w * 2.0f;
    float h = _h * 2.0f;

    Mesh mesh;
    mesh.addVertex(glm::vec3(x, y, 0.0));
    mesh.addColor(glm::vec4(1.0));
    mesh.addNormal(glm::vec3(0.0, 0.0, 1.0));
    mesh.addTexCoord(glm::vec2(0.0, 0.0));

    mesh.addVertex(glm::vec3(x+w, y, 0.0));
    mesh.addColor(glm::vec4(1.0));
    mesh.addNormal(glm::vec3(0.0, 0.0, 1.0));
    mesh.addTexCoord(glm::vec2(1.0, 0.0));

    mesh.addVertex(glm::vec3(x+w, y+h, 0.0));
    mesh.addColor(glm::vec4(1.0));
    mesh.addNormal(glm::vec3(0.0, 0.0, 1.0));
    mesh.addTexCoord(glm::vec2(1.0, 1.0));

    mesh.addVertex(glm::vec3(x, y+h, 0.0));
    mesh.addColor(glm::vec4(1.0));
    mesh.addNormal(glm::vec3(0.0, 0.0, 1.0));
    mesh.addTexCoord(glm::vec2(0.0, 1.0));

    mesh.addIndex(0);   mesh.addIndex(1);   mesh.addIndex(2);
    mesh.addIndex(2);   mesh.addIndex(3);   mesh.addIndex(0);

    return mesh;
}

Mesh planeMesh(float _width, float _height, int _columns, int _rows, DrawMode _drawMode) {
    Mesh mesh;

    if (_drawMode != TRIANGLE_STRIP && _drawMode != TRIANGLES)
        _drawMode = TRIANGLES;

    _columns++;
    _rows++;

    mesh.setDrawMode(_drawMode);

    glm::vec3 vert(0.0f, 0.0f, 0.0f);
    glm::vec3 normal(0.0f, 0.0f, 1.0f); // always facing forward //
    glm::vec2 texcoord(0.0f, 0.0f);

    // the origin of the plane is at the center //
    float halfW = _width  * 0.5f;
    float halfH = _height * 0.5f;
    
    // add the vertexes //
    for (int iy = 0; iy != _rows; iy++) {
        for (int ix = 0; ix != _columns; ix++) {

            // normalized tex coords //
            texcoord.x =       ((float)ix/((float)_columns-1));
            texcoord.y = 1.f - ((float)iy/((float)_rows-1));

            vert.x = texcoord.x * _width - halfW;
            vert.y = -(texcoord.y-1) * _height - halfH;

            mesh.addVertex(vert);
            mesh.addTexCoord(texcoord);
            mesh.addNormal(normal);
        }
    }

    if (_drawMode == TRIANGLE_STRIP) {
        for (int y = 0; y < _rows-1; y++) {
            // even _rows //
            if ((y&1)==0) {
                for (int x = 0; x < _columns; x++) {
                    mesh.addIndex( (y) * _columns + x );
                    mesh.addIndex( (y+1) * _columns + x);
                }
            }
            else {
                for (int x = _columns-1; x >0; x--) {
                    mesh.addIndex( (y+1) * _columns + x );
                    mesh.addIndex( y * _columns + x-1 );
                }
            }
        }

        if (_rows%2 != 0) mesh.addIndex(mesh.getVerticesTotal() - _columns);
    }
    else {

        // Triangles //
        for (int y = 0; y < _rows - 1; y++) {
            for (int x = 0; x < _columns - 1; x++) {
                // first triangle //
                mesh.addIndex((y) *_columns + x);
                mesh.addIndex((y) *_columns + x + 1);
                mesh.addIndex((y + 1) *_columns + x);

                // second triangle //
                mesh.addIndex((y) * _columns + x+1);
                mesh.addIndex((y + 1) *_columns + x+1);
                mesh.addIndex((y + 1) *_columns + x);
            }
        }
    }

    return mesh;
}


Mesh boxMesh( float _width, float _height, float _depth, int _resX, int _resY, int _resZ ) {

    // mesh only available as triangles //
    Mesh mesh;
    mesh.setDrawMode( TRIANGLES );

    _resX = _resX + 1;
    _resY = _resY + 1;
    _resZ = _resZ + 1;

    if ( _resX < 2 ) _resX = 0;
    if ( _resY < 2 ) _resY = 0;
    if ( _resZ < 2 ) _resZ = 0;

    // halves //
    float halfW = _width * .5f;
    float halfH = _height * .5f;
    float halfD = _depth * .5f;

    glm::vec3 vert;
    glm::vec3 normal;
    glm::vec2 texcoord;
    std::size_t vertOffset = 0;

    // TRIANGLES //

    // Front Face //
    normal = {0.f, 0.f, 1.f};
    // add the vertexes //
    for (int iy = 0; iy < _resY; iy++) {
        for (int ix = 0; ix < _resX; ix++) {

            // normalized tex coords //
            texcoord.x = ((float)ix/((float)_resX-1.f));
            texcoord.y = 1.f - ((float)iy/((float)_resY-1.f));

            vert.x = texcoord.x * _width - halfW;
            vert.y = -(texcoord.y-1.f) * _height - halfH;
            vert.z = halfD;

            mesh.addVertex(vert);
            mesh.addTexCoord(texcoord);
            mesh.addNormal(normal);
        }
    }

    for (int y = 0; y < _resY-1; y++) {
        for (int x = 0; x < _resX-1; x++) {
            // first triangle //
            mesh.addIndex((y)*_resX + x + vertOffset);
            mesh.addIndex((y)*_resX + x+1 + vertOffset);
            mesh.addIndex((y+1)*_resX + x + vertOffset);

            // second triangle //
            mesh.addIndex((y)*_resX + x+1 + vertOffset);
            mesh.addIndex((y+1)*_resX + x+1 + vertOffset);
            mesh.addIndex((y+1)*_resX + x + vertOffset);
        }
    }

    vertOffset = mesh.getVerticesTotal();

    // Right Side Face //
    normal = {1.f, 0.f, 0.f};
    // add the vertexes //
    for (int iy = 0; iy < _resY; iy++) {
        for (int ix = 0; ix < _resZ; ix++) {

            // normalized tex coords //
            texcoord.x = ((float)ix/((float)_resZ-1.f));
            texcoord.y = 1.f - ((float)iy/((float)_resY-1.f));

            //vert.x = texcoord.x * _width - halfW;
            vert.x = halfW;
            vert.y = -(texcoord.y-1.f) * _height - halfH;
            vert.z = texcoord.x * -_depth + halfD;

            mesh.addVertex(vert);
            mesh.addTexCoord(texcoord);
            mesh.addNormal(normal);
        }
    }

    for (int y = 0; y < _resY-1; y++) {
        for (int x = 0; x < _resZ-1; x++) {
            // first triangle //
            mesh.addIndex((y)*_resZ + x + vertOffset);
            mesh.addIndex((y)*_resZ + x+1 + vertOffset);
            mesh.addIndex((y+1)*_resZ + x + vertOffset);

            // second triangle //
            mesh.addIndex((y)*_resZ + x+1 + vertOffset);
            mesh.addIndex((y+1)*_resZ + x+1 + vertOffset);
            mesh.addIndex((y+1)*_resZ + x + vertOffset);
        }
    }

    vertOffset = mesh.getVerticesTotal();

    // Left Side Face //
    normal = {-1.f, 0.f, 0.f};
    // add the vertexes //
    for (int iy = 0; iy < _resY; iy++) {
        for (int ix = 0; ix < _resZ; ix++) {

            // normalized tex coords //
            texcoord.x = ((float)ix/((float)_resZ-1.f));
            texcoord.y = 1.f-((float)iy/((float)_resY-1.f));

            //vert.x = texcoord.x * _width - halfW;
            vert.x = -halfW;
            vert.y = -(texcoord.y-1.f) * _height - halfH;
            vert.z = texcoord.x * _depth - halfD;

            mesh.addVertex(vert);
            mesh.addTexCoord(texcoord);
            mesh.addNormal(normal);
        }
    }

    for (int y = 0; y < _resY-1; y++) {
        for (int x = 0; x < _resZ-1; x++) {
            // first triangle //
            mesh.addIndex((y)*_resZ + x + vertOffset);
            mesh.addIndex((y)*_resZ + x+1 + vertOffset);
            mesh.addIndex((y+1)*_resZ + x + vertOffset);

            // second triangle //
            mesh.addIndex((y)*_resZ + x+1 + vertOffset);
            mesh.addIndex((y+1)*_resZ + x+1 + vertOffset);
            mesh.addIndex((y+1)*_resZ + x + vertOffset);
        }
    }

    vertOffset = mesh.getVerticesTotal();

    // Back Face //
    normal = {0.f, 0.f, -1.f};
    // add the vertexes //
    for (int iy = 0; iy < _resY; iy++) {
        for (int ix = 0; ix < _resX; ix++) {

            // normalized tex coords //
            texcoord.x = ((float)ix/((float)_resX-1.f));
            texcoord.y = 1.f-((float)iy/((float)_resY-1.f));

            vert.x = texcoord.x * -_width + halfW;
            vert.y = -(texcoord.y-1.f) * _height - halfH;
            vert.z = -halfD;

            mesh.addVertex(vert);
            mesh.addTexCoord(texcoord);
            mesh.addNormal(normal);
        }
    }

    for (int y = 0; y < _resY-1; y++) {
        for (int x = 0; x < _resX-1; x++) {
            // first triangle //
            mesh.addIndex((y)*_resX + x + vertOffset);
            mesh.addIndex((y)*_resX + x+1 + vertOffset);
            mesh.addIndex((y+1)*_resX + x + vertOffset);

            // second triangle //
            mesh.addIndex((y)*_resX + x+1 + vertOffset);
            mesh.addIndex((y+1)*_resX + x+1 + vertOffset);
            mesh.addIndex((y+1)*_resX + x + vertOffset);
        }
    }

    vertOffset = mesh.getVerticesTotal();


    // Top Face //
    normal = {0.f, -1.f, 0.f};
    // add the vertexes //
    for (int iy = 0; iy < _resZ; iy++) {
        for (int ix = 0; ix < _resX; ix++) {

            // normalized tex coords //
            texcoord.x = ((float)ix/((float)_resX-1.f));
            texcoord.y = 1.f-((float)iy/((float)_resZ-1.f));

            vert.x = texcoord.x * _width - halfW;
            //vert.y = -(texcoord.y-1.f) * _height - halfH;
            vert.y = -halfH;
            vert.z = texcoord.y * _depth - halfD;

            mesh.addVertex(vert);
            mesh.addTexCoord(texcoord);
            mesh.addNormal(normal);
        }
    }

    for (int y = 0; y < _resZ-1; y++) {
        for (int x = 0; x < _resX-1; x++) {
            // first triangle //
            mesh.addIndex((y)*_resX + x + vertOffset);
            mesh.addIndex((y+1)*_resX + x + vertOffset);
            mesh.addIndex((y)*_resX + x+1 + vertOffset);

            // second triangle //
            mesh.addIndex((y)*_resX + x+1 + vertOffset);
            mesh.addIndex((y+1)*_resX + x + vertOffset);
            mesh.addIndex((y+1)*_resX + x+1 + vertOffset);
        }
    }

    vertOffset = mesh.getVerticesTotal();

    // Bottom Face //
    normal = {0.f, 1.f, 0.f};
    // add the vertexes //
    for (int iy = 0; iy < _resZ; iy++) {
        for (int ix = 0; ix < _resX; ix++) {

            // normalized tex coords //
            texcoord.x = ((float)ix/((float)_resX-1.f));
            texcoord.y = 1.f-((float)iy/((float)_resZ-1.f));

            vert.x = texcoord.x * _width - halfW;
            //vert.y = -(texcoord.y-1.f) * _height - halfH;
            vert.y = halfH;
            vert.z = texcoord.y * -_depth + halfD;

            mesh.addVertex(vert);
            mesh.addTexCoord(texcoord);
            mesh.addNormal(normal);
        }
    }

    for (int y = 0; y < _resZ-1; y++) {
        for (int x = 0; x < _resX-1; x++) {
            // first triangle //
            mesh.addIndex((y)*_resX + x + vertOffset);
            mesh.addIndex((y+1)*_resX + x + vertOffset);
            mesh.addIndex((y)*_resX + x+1 + vertOffset);

            // second triangle //
            mesh.addIndex((y)*_resX + x+1 + vertOffset);
            mesh.addIndex((y+1)*_resX + x + vertOffset);
            mesh.addIndex((y+1)*_resX + x+1 + vertOffset);
        }
    }
    mesh.computeTangents();

    return mesh;
}


Mesh cubeMesh( float _size, int _resolution) {
    return boxMesh(_size, _size, _size, _resolution, _resolution, _resolution);
}

Mesh cubeMesh(float _size) {
    float vertices[] = {
        -_size,  _size,  _size,
        -_size, -_size,  _size,
         _size, -_size,  _size,
         _size,  _size,  _size,
        -_size,  _size, -_size,
        -_size, -_size, -_size,
         _size, -_size, -_size,
         _size,  _size, -_size,
    };

    INDEX_TYPE indices[] = {
        0, 1, 2,
        0, 2, 3,
        3, 2, 6,
        3, 6, 7,
        0, 4, 7,
        0, 7, 3,
        4, 6, 7,
        4, 6, 5,
        0, 5, 4,
        0, 5, 1,
        1, 6, 5,
        1, 6, 2,
    };

    Mesh mesh;
    mesh.addVertices(reinterpret_cast<glm::vec3*>(vertices), 8);
    mesh.addIndices(indices, 36);
    return mesh;
}

Mesh cubeCornersMesh(const std::vector<glm::vec3> &_pts, float _size) {
    BoundingBox bbox = getBoundingBox(_pts);
    return cubeCornersMesh(bbox.min, bbox.max, _size);
}

Mesh cubeCornersMesh(const BoundingBox& _bbox, float _size) {
    return cubeCornersMesh(_bbox.min, _bbox.max, _size);
}

Mesh cubeCornersMesh(const glm::vec3 &_min_v, const glm::vec3 &_max_v, float _size) {
    float size = glm::min(glm::length(_min_v), glm::length(_max_v)) * _size *  0.5;

    //    D ---- A
    // C ---- B  |
    // |  |   |  |
    // |  I --|- F
    // H .... G

    glm::vec3 A = _max_v;
    glm::vec3 H = _min_v;

    glm::vec3 B = glm::vec3(A.x, A.y, H.z);
    glm::vec3 C = glm::vec3(H.x, A.y, H.z);
    glm::vec3 D = glm::vec3(H.x, A.y, A.z);

    glm::vec3 F = glm::vec3(A.x, H.y, A.z);
    glm::vec3 G = glm::vec3(A.x, H.y, H.z);
    glm::vec3 I = glm::vec3(H.x, H.y, A.z);

    Mesh mesh;
    mesh.setDrawMode(LINES);
    mesh.append( lineToMesh(A, normalize(D-A), size) );
    mesh.append( lineToMesh(A, normalize(B-A), size) );
    mesh.append( lineToMesh(A, normalize(F-A), size) );

    mesh.append( lineToMesh(B, normalize(A-B), size) );
    mesh.append( lineToMesh(B, normalize(C-B), size) );
    mesh.append( lineToMesh(B, normalize(G-B), size) );

    mesh.append( lineToMesh(C, normalize(D-C), size) );
    mesh.append( lineToMesh(C, normalize(B-C), size) );
    mesh.append( lineToMesh(C, normalize(H-C), size) );
    
    mesh.append( lineToMesh(D, normalize(A-D), size) );
    mesh.append( lineToMesh(D, normalize(C-D), size) );
    mesh.append( lineToMesh(D, normalize(I-D), size) );

    mesh.append( lineToMesh(F, normalize(G-F), size) );
    mesh.append( lineToMesh(F, normalize(A-F), size) );
    mesh.append( lineToMesh(F, normalize(I-F), size) );

    mesh.append( lineToMesh(G, normalize(H-G), size) );
    mesh.append( lineToMesh(G, normalize(F-G), size) );
    mesh.append( lineToMesh(G, normalize(B-G), size) );

    mesh.append( lineToMesh(H, normalize(I-H), size) );
    mesh.append( lineToMesh(H, normalize(G-H), size) );
    mesh.append( lineToMesh(H, normalize(C-H), size) );

    mesh.append( lineToMesh(I, normalize(F-I), size) );
    mesh.append( lineToMesh(I, normalize(H-I), size) );
    mesh.append( lineToMesh(I, normalize(D-I), size) );

    return mesh;
}

Mesh axisMesh(float _size, float _y) {
    Mesh mesh;
    mesh.setDrawMode(LINES);

    mesh.append( lineMesh(glm::vec3(_size,_y,0.0), glm::vec3(-_size,_y,0.0)));
    mesh.append( lineMesh(glm::vec3(0.0, _size, 0.0), glm::vec3(0.0, -_size, 0.0)));
    mesh.append( lineMesh(glm::vec3(0.0, _y, _size), glm::vec3(0.0, _y, -_size)));

    return mesh;
}

Mesh gridMesh(float _width, float _height, int _columns, int _rows, float _y) {
    Mesh mesh;
    mesh.setDrawMode(LINES);

    // the origin of the plane is at the center //
    float halfW = _width  * 0.5f;
    float halfH = _height * 0.5f;

    //  . --- A
    //  |     |
    //  B --- .

    glm::vec3 A = glm::vec3(halfW, _y, halfH);
    glm::vec3 B = glm::vec3(-halfW, _y, -halfH);

    // add the vertexes //
    for(int iy = 0; iy != _rows; iy++) {
        float pct = ((float)iy/((float)_rows-1));

        glm::vec3 left = glm::mix(A, B, glm::vec3(0.0, _y, pct));
        glm::vec3 right = glm::mix(A, B, glm::vec3(1.0, _y, pct));

        mesh.append( lineMesh(left, right) );
    }

    for(int ix = 0; ix != _columns; ix++) {
        float pct = ((float)ix/((float)_columns-1));

        glm::vec3 top = glm::mix(A, B, glm::vec3(pct, _y, 0.0));
        glm::vec3 down = glm::mix(A, B, glm::vec3(pct, _y, 1.0));

        mesh.append( lineMesh(top, down) );
    }

    return mesh;
}

Mesh gridMesh(float _size, int _segments, float _y) {
    return gridMesh(_size, _size, _segments, _segments, _y);
}

Mesh floorMesh(float _area, int _subD, float _y) {

    int N = pow(2,_subD);

    Mesh mesh;
    float w = _area/float(N);
    float h = _area/2.0;
    for (int z = 0; z <= N; z++){
        for (int x = 0; x <= N; x++){
            mesh.addVertex(glm::vec3(x * w - h, _y, z * w - h));
            mesh.addColor(glm::vec4(0.251, 0.251, 0.251, 1.0));
            mesh.addNormal(glm::vec3(0.0, 1.0, 0.0));
            mesh.addTexCoord(glm::vec2(float(x)/float(N), float(z)/float(N)));
        }
    }
    
    //
    // 0 -- 1 -- 2      A -- B
    // |    |    |      |    | 
    // 3 -- 4 -- 5      C -- D
    // |    |    |
    // 6 -- 7 -- 8
    //
    for (int y = 0; y < N; y++){
        for (int x=0; x < N; x++){
            mesh.addIndex(  x   +   y   * (N+1));   // A
            mesh.addIndex((x+1) +   y   * (N+1));   // B
            mesh.addIndex((x+1) + (y+1) * (N+1));   // D

            mesh.addIndex((x+1) + (y+1) * (N+1));   // D
            mesh.addIndex(  x   + (y+1) * (N+1));   // C
            mesh.addIndex(  x   +   y   * (N+1));   // A
        }
    }
    mesh.computeTangents();

    return mesh;
}

Mesh sphereMesh(int _resolution, float _radius, DrawMode _drawMode) {
    Mesh mesh;

    float doubleRes = _resolution*2.f;
    float polarInc = PI/(_resolution); // ringAngle
    float azimInc = TAU/(doubleRes); // segAngle

    if (_drawMode != TRIANGLE_STRIP && _drawMode != TRIANGLES)
        _drawMode = TRIANGLE_STRIP;
    mesh.setDrawMode(_drawMode);
    
    glm::vec3 vert;
    glm::vec2 tcoord;

    for (float i = 0; i < _resolution + 1; i++) {

        float tr = sin( PI-i * polarInc );
        float ny = cos( PI-i * polarInc );

        tcoord.y = 1.f - (i / _resolution);

        for (float j = 0; j <= doubleRes; j++) {

            float nx = tr * sin(j * azimInc);
            float nz = tr * cos(j * azimInc);

            tcoord.x = j / (doubleRes);

            vert = {nx, ny, nz};
            mesh.addNormal(vert);

            vert *= _radius;
            mesh.addVertex(vert);
            mesh.addTexCoord(tcoord);
        }
    }

    int nr = doubleRes+1;
    
    if (_drawMode == TRIANGLES) {
        int index1, index2, index3;
        for (float iy = 0; iy < _resolution; iy++) {
            for (float ix = 0; ix < doubleRes; ix++) {

                // first tri //
                if (iy > 0) {
                    index1 = (iy+0) * (nr) + (ix+0);
                    index2 = (iy+0) * (nr) + (ix+1);
                    index3 = (iy+1) * (nr) + (ix+0);

                    mesh.addIndex(index1);
                    mesh.addIndex(index2);
                    mesh.addIndex(index3);
                }

                if (iy < _resolution - 1 ) {
                    // second tri //
                    index1 = (iy+0) * (nr) + (ix+1);
                    index2 = (iy+1) * (nr) + (ix+1);
                    index3 = (iy+1) * (nr) + (ix+0);

                    mesh.addIndex(index1);
                    mesh.addIndex(index2);
                    mesh.addIndex(index3);

                }
            }
        }
    }
    else {
        for (int y = 0; y < _resolution; y++) {
            for (int x = 0; x <= doubleRes; x++) {
                mesh.addIndex( (y)*nr + x );
                mesh.addIndex( (y+1)*nr + x );
            }
        }
    }
    mesh.computeTangents();

    return mesh;
}


Mesh sphereHalfMesh(int _resolution, float _radius ) {
    Mesh mesh;

    float halfRes = _resolution*.5f;
    float doubleRes = _resolution*2.f;
    float polarInc = PI/(_resolution); // ringAngle
    float azimInc = TAU/(doubleRes); // segAngle //
    
    glm::vec3 vert;
    glm::vec2 tcoord;

    for (float i = halfRes; i < _resolution + 1; i++) {
        float tr = sin( PI-i * polarInc);
        float ny = cos( PI-i * polarInc);
        tcoord.y = 1.f - (i / _resolution);

        for (float j = 0; j <= doubleRes; j++) {

            float nx = tr * sin(j * azimInc);
            float nz = tr * cos(j * azimInc);

            tcoord.x = j / (doubleRes);

            vert = {nx, ny, nz};
            mesh.addNormal(vert);

            vert *= _radius;
            mesh.addVertex(vert);
            mesh.addTexCoord(tcoord);
        }
    }

    int nr = doubleRes+1;
    
    int index1, index2, index3;
    for (float iy = halfRes; iy < _resolution; iy++) {
        for (float ix = 0; ix < doubleRes; ix++) {

            // first tri //
            if (iy > 0) {
                index1 = (iy+0-halfRes) * (nr) + (ix+0);
                index2 = (iy+0-halfRes) * (nr) + (ix+1);
                index3 = (iy+1-halfRes) * (nr) + (ix+0);

                mesh.addIndex(index1);
                mesh.addIndex(index2);
                mesh.addIndex(index3);
            }

            if (iy < _resolution - 1 ) {
                // second tri //
                index1 = (iy+0-halfRes) * (nr) + (ix+1);
                index2 = (iy+1-halfRes) * (nr) + (ix+1);
                index3 = (iy+1-halfRes) * (nr) + (ix+0);

                mesh.addIndex(index1);
                mesh.addIndex(index2);
                mesh.addIndex(index3);

            }
        }
    }
    mesh.computeTangents();

    return mesh;
}


Mesh icosphereMesh(float _radius, size_t _iterations) {

    /// Step 1 : Generate icosahedron
    const float sqrt5 = sqrt(5.0f);
    const float phi = (1.0f + sqrt5) * 0.5f;
    const float invnorm = 1/sqrt(phi*phi+1);

    std::vector<glm::vec3> vertices;
    vertices.push_back(invnorm * glm::vec3(-1,  phi, 0));//0
    vertices.push_back(invnorm * glm::vec3( 1,  phi, 0));//1
    vertices.push_back(invnorm * glm::vec3(0,   1,  -phi));//2
    vertices.push_back(invnorm * glm::vec3(0,   1,   phi));//3
    vertices.push_back(invnorm * glm::vec3(-phi,0,  -1));//4
    vertices.push_back(invnorm * glm::vec3(-phi,0,   1));//5
    vertices.push_back(invnorm * glm::vec3( phi,0,  -1));//6
    vertices.push_back(invnorm * glm::vec3( phi,0,   1));//7
    vertices.push_back(invnorm * glm::vec3(0,   -1, -phi));//8
    vertices.push_back(invnorm * glm::vec3(0,   -1,  phi));//9
    vertices.push_back(invnorm * glm::vec3(-1,  -phi,0));//10
    vertices.push_back(invnorm * glm::vec3( 1,  -phi,0));//11
       
    std::vector<INDEX_TYPE> indices = {
        0,1,2,
        0,3,1,
        0,4,5,
        1,7,6,
        1,6,2,
        1,3,7,
        0,2,4,
        0,5,3,
        2,6,8,
        2,8,4,
        3,5,9,
        3,9,7,
        11,6,7,
        10,5,4,
        10,4,8,
        10,9,5,
        11,8,6,
        11,7,9,
        10,8,11,
        10,11,9
    };

    size_t size = indices.size();

    /// Step 2 : tessellate
    for (size_t iteration = 0; iteration < _iterations; iteration++) {
        size*=4;
        std::vector<INDEX_TYPE> newFaces;
        for (size_t i = 0; i < size/12; i++) {
            INDEX_TYPE i1 = indices[i*3];
            INDEX_TYPE i2 = indices[i*3+1];
            INDEX_TYPE i3 = indices[i*3+2];
            size_t i12 = vertices.size();
            size_t i23 = i12+1;
            size_t i13 = i12+2;
            glm::vec3 v1 = vertices[i1];
            glm::vec3 v2 = vertices[i2];
            glm::vec3 v3 = vertices[i3];
            //make 1 vertice at the center of each edge and project it onto the sphere
            vertices.push_back(glm::normalize(v1+v2));
            vertices.push_back(glm::normalize(v2+v3));
            vertices.push_back(glm::normalize(v1+v3));
            //now recreate indices
            newFaces.push_back(i1);
            newFaces.push_back(i12);
            newFaces.push_back(i13);
            newFaces.push_back(i2);
            newFaces.push_back(i23);
            newFaces.push_back(i12);
            newFaces.push_back(i3);
            newFaces.push_back(i13);
            newFaces.push_back(i23);
            newFaces.push_back(i12);
            newFaces.push_back(i23);
            newFaces.push_back(i13);
        }
        indices.swap(newFaces);
    }

    /// Step 3 : generate texcoords
    std::vector<glm::vec2> texCoords;
    for (size_t i = 0; i < vertices.size(); i++) {
        const auto& vec = vertices[i];
        float u, v;
        float r0 = sqrtf(vec.x*vec.x+vec.z*vec.z);
        float alpha;
        alpha = atan2f(vec.z,vec.x);
        u = alpha/TAU+.5f;
        v = atan2f(vec.y, r0)/PI + .5f;
        // reverse the u coord, so the default is texture mapped left to
        // right on the outside of a sphere 
        // reverse the v coord, so that texture origin is at top left
        texCoords.push_back(glm::vec2(1.0-u,1.f-v));
    }

    /// Step 4 : fix texcoords
    // find vertices to split
    std::vector<int> indexToSplit;

    for (size_t i=0; i<indices.size()/3; i++) {
        glm::vec2 t0 = texCoords[indices[i*3+0]];
        glm::vec2 t1 = texCoords[indices[i*3+1]];
        glm::vec2 t2 = texCoords[indices[i*3+2]];

        if (std::abs(t2.x-t0.x)>0.5) {
            if (t0.x<0.5)
                indexToSplit.push_back(indices[i*3]);
            else
                indexToSplit.push_back(indices[i*3+2]);
        }
        if (std::abs(t1.x-t0.x)>0.5) {
            if (t0.x<0.5)
                indexToSplit.push_back(indices[i*3]);
            else
                indexToSplit.push_back(indices[i*3+1]);
        }
        if (std::abs(t2.x-t1.x)>0.5) {
            if (t1.x<0.5)
                indexToSplit.push_back(indices[i*3+1]);
            else
                indexToSplit.push_back(indices[i*3+2]);
        }
    }

    //split vertices
    for (size_t i = 0; i < indexToSplit.size(); i++) {
        INDEX_TYPE index = indexToSplit[i];
        //duplicate vertex
        glm::vec3 v = vertices[index];
        glm::vec2 t = texCoords[index] + glm::vec2(1.f, 0.f);
        vertices.push_back(v);
        texCoords.push_back(t);
        size_t newIndex = vertices.size()-1;
        //reassign indices
        for (size_t j = 0; j<indices.size(); j++) {
            if (indices[j] == index) {
                INDEX_TYPE index1 = indices[(j+1)%3+(j/3)*3];
                INDEX_TYPE index2 = indices[(j+2)%3+(j/3)*3];
                if ((texCoords[index1].x>0.5) || (texCoords[index2].x>0.5))
                    indices[j] = newIndex;
            }
        }
    }

    Mesh mesh;
    mesh.addNormals( vertices );
    mesh.addTexCoords( texCoords );

    for (size_t i = 0; i < vertices.size(); i++ )
        vertices[i] *= _radius;

    mesh.addVertices( vertices );
    mesh.addIndices( indices );
    mesh.computeTangents();

    return  mesh;
}

Mesh cylinder( float _radius, float _height, int _radiusSegments, int _heightSegments, int _numCapSegments, bool _bCapped, DrawMode _drawMode ) {
    Mesh mesh;

    if (_drawMode != TRIANGLE_STRIP && _drawMode != TRIANGLES)
        _drawMode = TRIANGLE_STRIP;
        
    mesh.setDrawMode(_drawMode);

    _radiusSegments = _radiusSegments+1;
    int capSegs = _numCapSegments;
    capSegs = capSegs+1;
    _heightSegments = _heightSegments+1;
    if (_heightSegments < 2) _heightSegments = 2;
    if ( capSegs < 2 ) _bCapped = false;
    if (!_bCapped) capSegs=1;

    float angleIncRadius = -1 * (TAU/((float)_radiusSegments-1.f));
    float heightInc = _height/((float)_heightSegments-1.f);
    float halfH = _height*.5f;

    float newRad;
    glm::vec3 vert;
    glm::vec2 tcoord;
    glm::vec3 normal;
    glm::vec3 up(0,1,0);

    std::size_t vertOffset = 0;

    float maxTexY   = _heightSegments-1.f;
    if (capSegs > 0)
        maxTexY += (capSegs*2)-2.f;
        
    float maxTexYNormalized = (capSegs-1.f) / maxTexY;

    // add the top cap //
    if (_bCapped && capSegs > 0) {
        normal = {0.f, -1.f, 0.f};
        for (int iy = 0; iy < capSegs; iy++) {
            for (int ix = 0; ix < _radiusSegments; ix++) {
                newRad = remap((float)iy, 0, capSegs-1, 0.0, _radius, true);
                vert.x = cos((float)ix*angleIncRadius) * newRad;
                vert.z = sin((float)ix*angleIncRadius) * newRad;
                vert.y = -halfH;

                tcoord.x = (float)ix/((float)_radiusSegments-1.f);
                tcoord.y = 1.f - remap(iy, 0, capSegs-1, 0, maxTexYNormalized, true);

                mesh.addTexCoord( tcoord );
                mesh.addVertex( vert );
                mesh.addNormal( normal );
            }
        }

        if (_drawMode == TRIANGLES) {
            for (int y = 0; y < capSegs-1; y++) {
                for (int x = 0; x < _radiusSegments-1; x++) {
                    if (y > 0) {
                        // first triangle //
                        mesh.addIndex( (y)*_radiusSegments + x + vertOffset );
                        mesh.addIndex( (y)*_radiusSegments + x+1 + vertOffset);
                        mesh.addIndex( (y+1)*_radiusSegments + x + vertOffset);
                    }

                    // second triangle //
                    mesh.addIndex( (y)*_radiusSegments + x+1 + vertOffset);
                    mesh.addIndex( (y+1)*_radiusSegments + x+1 + vertOffset);
                    mesh.addIndex( (y+1)*_radiusSegments + x + vertOffset);
                }
            }
        } else {
            for (int y = 0; y < capSegs-1; y++) {
                for (int x = 0; x < _radiusSegments; x++) {
                    mesh.addIndex( (y)*_radiusSegments + x + vertOffset );
                    mesh.addIndex( (y+1)*_radiusSegments + x + vertOffset);
                }
            }
        }

        vertOffset = mesh.getVerticesTotal();

    }

    //maxTexY            = _heightSegments-1.f + capSegs-1.f;
    float minTexYNormalized = 0;
    if (_bCapped) minTexYNormalized = maxTexYNormalized;
    maxTexYNormalized   = 1.f;
    if (_bCapped) maxTexYNormalized = (_heightSegments) / maxTexY;

    // cylinder vertices //
    for (int iy = 0; iy < _heightSegments; iy++) {
        normal = {1.f, 0.f, 0.f};
        for (int ix = 0; ix < _radiusSegments; ix++) {

            //newRad = remap((float)iy, 0, _heightSegments-1, 0.0, radius);
            vert.x = cos(ix*angleIncRadius) * _radius;
            vert.y = heightInc*float(iy) - halfH;
            vert.z = sin(ix*angleIncRadius) * _radius;

            tcoord.x = float(ix)/(float(_radiusSegments)-1.f);
            tcoord.y = 1.f - remap(iy, 0, _heightSegments-1, minTexYNormalized, maxTexYNormalized, true );

            mesh.addTexCoord( tcoord );
            mesh.addVertex( vert );
            mesh.addNormal( normal );

            normal = glm::rotate(normal, -angleIncRadius, up);

        }
    }

    if (_drawMode == TRIANGLES) {
        for (int y = 0; y < _heightSegments-1; y++) {
            for (int x = 0; x < _radiusSegments-1; x++) {
                // first triangle //
                mesh.addIndex( (y)*_radiusSegments + x + vertOffset);
                mesh.addIndex( (y)*_radiusSegments + x+1 + vertOffset );
                mesh.addIndex( (y+1)*_radiusSegments + x + vertOffset );

                // second triangle //
                mesh.addIndex( (y)*_radiusSegments + x+1 + vertOffset );
                mesh.addIndex( (y+1)*_radiusSegments + x+1 + vertOffset );
                mesh.addIndex( (y+1)*_radiusSegments + x + vertOffset );
            }
        }
    } else {
        for (int y = 0; y < _heightSegments-1; y++) {
            for (int x = 0; x < _radiusSegments; x++) {
                mesh.addIndex( (y)*_radiusSegments + x + vertOffset );
                mesh.addIndex( (y+1)*_radiusSegments + x + vertOffset );
            }
        }
    }

    vertOffset = mesh.getVerticesTotal();

    // add the bottom cap
    if (_bCapped && capSegs > 0) {
        minTexYNormalized = maxTexYNormalized;
        maxTexYNormalized   = 1.f;

        normal = {0.f, 1.f, 0.f};
        for (int iy = 0; iy < capSegs; iy++) {
            for (int ix = 0; ix < _radiusSegments; ix++) {
                newRad = remap((float)iy, 0, capSegs-1, _radius, 0.0, true);
                vert.x = cos((float)ix*angleIncRadius) * newRad;
                vert.z = sin((float)ix*angleIncRadius) * newRad;
                vert.y = halfH;

                tcoord.x = (float)ix/((float)_radiusSegments-1.f);
                tcoord.y = 1.f - remap(iy, 0, capSegs-1, minTexYNormalized, maxTexYNormalized, true);

                mesh.addTexCoord( tcoord );
                mesh.addVertex( vert );
                mesh.addNormal( normal );
            }
        }

        if (_drawMode == TRIANGLES) {
            for (int y = 0; y < capSegs-1; y++) {
                for (int x = 0; x < _radiusSegments-1; x++) {
                    // first triangle //
                    mesh.addIndex( (y)*_radiusSegments + x + vertOffset );
                    mesh.addIndex( (y)*_radiusSegments + x+1 + vertOffset);
                    mesh.addIndex( (y+1)*_radiusSegments + x + vertOffset);

                    if (y < capSegs -1 && capSegs > 2) {
                        // second triangle //
                        mesh.addIndex( (y)*_radiusSegments + x+1 + vertOffset);
                        mesh.addIndex( (y+1)*_radiusSegments + x+1 + vertOffset);
                        mesh.addIndex( (y+1)*_radiusSegments + x + vertOffset);
                    }
                }
            }
        } else {
            for (int y = 0; y < capSegs-1; y++) {
                for (int x = 0; x < _radiusSegments; x++) {
                    mesh.addIndex( (y)*_radiusSegments + x + vertOffset );
                    mesh.addIndex( (y+1)*_radiusSegments + x + vertOffset);
                }
            }
        }

        vertOffset = mesh.getVerticesTotal();

    }

    return mesh;
}

Mesh cone( float radius, float _height, int _radiusSegments, int _heightSegments, int _capSegments, DrawMode _drawMode ) {
    Mesh mesh;
    if (_drawMode != TRIANGLE_STRIP && _drawMode != TRIANGLES)
        _drawMode = TRIANGLE_STRIP;

    mesh.setDrawMode(_drawMode);

    _radiusSegments = _radiusSegments+1;
    _capSegments = _capSegments+1;
    _heightSegments = _heightSegments+1;
    if (_heightSegments < 2) _heightSegments = 2;
    int capSegs = _capSegments;
    if ( capSegs < 2 )
        capSegs = 0;

    float angleIncRadius = -1.f * ((TAU/((float)_radiusSegments-1.f)));
    float heightInc = _height/((float)_heightSegments-1);
    float halfH = _height*.5f;

    float newRad;
    glm::vec3 vert;
    glm::vec3 normal;
    glm::vec2 tcoord;
    glm::vec3 up(0.0,1.0,0.0);

    std::size_t vertOffset = 0;

    float maxTexY = _heightSegments-1.f;
    if (capSegs > 0) 
        maxTexY += capSegs-1.f;

    glm::vec3 startVec(0, -halfH-1.f, 0);

    // cone vertices //
    for (int iy = 0; iy < _heightSegments; iy++) {
        for (int ix = 0; ix < _radiusSegments; ix++) {

            newRad = remap((float)iy, 0, _heightSegments-1, 0.0, radius, true);
            vert.x = cos((float)ix*angleIncRadius) * newRad;
            vert.y = heightInc*((float)iy) - halfH;
            vert.z = sin((float)ix*angleIncRadius) * newRad;

            tcoord.x = (float)ix/((float)_radiusSegments-1.f);
            tcoord.y = 1.f - (float)iy/((float)maxTexY);

            mesh.addTexCoord( tcoord );
            mesh.addVertex( vert );

            if (iy == 0) {
                newRad = 1.f;
                vert.x = cos((float)ix*angleIncRadius) * newRad;
                vert.y = heightInc*((float)iy) - halfH;
                vert.z = sin((float)ix*angleIncRadius) * newRad;
            }

            glm::vec3 diff = vert - startVec;
            glm::vec3 crossed = glm::cross(up, vert);
            normal = glm::cross(crossed, diff);
            mesh.addNormal( glm::normalize(normal) );

        }
    }

    if (_drawMode == TRIANGLES) {
        for (int y = 0; y < _heightSegments-1; y++) {
            for (int x = 0; x < _radiusSegments-1; x++) {
                if (y > 0){
                    // first triangle //
                    mesh.addIndex( (y)*_radiusSegments + x );
                    mesh.addIndex( (y)*_radiusSegments + x+1 );
                    mesh.addIndex( (y+1)*_radiusSegments + x );
                }

                // second triangle //
                mesh.addIndex( (y)*_radiusSegments + x+1 );
                mesh.addIndex( (y+1)*_radiusSegments + x+1 );
                mesh.addIndex( (y+1)*_radiusSegments + x );
            }
        }
    }
    else {
        for (int y = 0; y < _heightSegments-1; y++) {
            for (int x = 0; x < _radiusSegments; x++) {
                mesh.addIndex( (y)*_radiusSegments + x );
                mesh.addIndex( (y+1)*_radiusSegments + x );
            }
        }
    }

    vertOffset = mesh.getVerticesTotal();
    float maxTexYNormalized = (_heightSegments-1.f) / maxTexY;

    // add the cap //
    normal = glm::vec3(0.f, 1.f, 0.f);
    for (int iy = 0; iy < capSegs; iy++) {
        for (int ix = 0; ix < _radiusSegments; ix++) {
            newRad = remap((float)iy, 0, capSegs-1, radius, 0.0, true);
            vert.x = cos((float)ix*angleIncRadius) * newRad;
            vert.z = sin((float)ix*angleIncRadius) * newRad;
            vert.y = halfH;

            tcoord.x = (float)ix/((float)_radiusSegments-1.f);
            tcoord.y = 1.f - remap(iy, 0, capSegs-1, maxTexYNormalized, 1.f, true);

            mesh.addTexCoord( tcoord );
            mesh.addVertex( vert );
            mesh.addNormal( normal );
        }
    }

    if (_drawMode == TRIANGLES) {
        if ( capSegs > 0 ) {
            for (int y = 0; y < capSegs-1; y++) {
                for (int x = 0; x < _radiusSegments-1; x++) {

                    // first triangle //
                    mesh.addIndex( (y)*_radiusSegments + x + vertOffset );
                    mesh.addIndex( (y)*_radiusSegments + x+1 + vertOffset);
                    mesh.addIndex( (y+1)*_radiusSegments + x + vertOffset);

                    if (y < capSegs-2) {
                        // second triangle //
                        mesh.addIndex( (y)*_radiusSegments + x+1 + vertOffset);
                        mesh.addIndex( (y+1)*_radiusSegments + x+1 + vertOffset);
                        mesh.addIndex( (y+1)*_radiusSegments + x + vertOffset);
                    }
                }
            }
        }
    }
    else {
        if (capSegs > 0 ) {
            for (int y = 0; y < capSegs-1; y++) {
                for (int x = 0; x < _radiusSegments; x++) {
                    mesh.addIndex( (y)*_radiusSegments + x + vertOffset );
                    mesh.addIndex( (y+1)*_radiusSegments + x + vertOffset);
                }
            }
        }
    }

    return mesh;
}


//  Triangulator by Michael Fogleman ( @FogleBird )
//  https://github.com/fogleman/hmm/blob/master/src/triangulator.cpp
//  All code and credits are for his genius. I took the code and make 
//  it significantly uglier using lambdas for my own conviniance
//

std::pair<glm::ivec2, float> FindCandidate( const Image& _image, const glm::ivec2 p0, const glm::ivec2 p1, const glm::ivec2 p2) { 
    
    const auto edge = []( const glm::ivec2 a, const glm::ivec2 b, const glm::ivec2 c) {
        return (b.x - c.x) * (a.y - c.y) - (b.y - c.y) * (a.x - c.x);
    };

    // triangle bounding box
    const glm::ivec2 min = glm::min(glm::min(p0, p1), p2);
    const glm::ivec2 max = glm::max(glm::max(p0, p1), p2);

    // forward differencing variables
    int w00 = edge(p1, p2, min);
    int w01 = edge(p2, p0, min);
    int w02 = edge(p0, p1, min);
    const int a01 = p1.y - p0.y;
    const int b01 = p0.x - p1.x;
    const int a12 = p2.y - p1.y;
    const int b12 = p1.x - p2.x;
    const int a20 = p0.y - p2.y;
    const int b20 = p2.x - p0.x;

    // pre-multiplied z values at vertices
    const float a = edge(p0, p1, p2);
    const float z0 = _image.getValue( _image.getIndex(p0.x, p0.y) ) / a;
    const float z1 = _image.getValue( _image.getIndex(p1.x, p1.y) ) / a;
    const float z2 = _image.getValue( _image.getIndex(p2.x, p2.y) ) / a;

    // iterate over pixels in bounding box
    float maxError = 0;
    glm::ivec2 maxPoint(0);
    for (int y = min.y; y <= max.y; y++) {
        // compute starting offset
        int dx = 0;
        if (w00 < 0 && a12 != 0)
            dx = std::max(dx, -w00 / a12);
        if (w01 < 0 && a20 != 0)
            dx = std::max(dx, -w01 / a20);
        if (w02 < 0 && a01 != 0)
            dx = std::max(dx, -w02 / a01);

        int w0 = w00 + a12 * dx;
        int w1 = w01 + a20 * dx;
        int w2 = w02 + a01 * dx;

        bool wasInside = false;

        for (int x = min.x + dx; x <= max.x; x++) {
            // check if inside triangle
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                wasInside = true;

                // compute z using barycentric coordinates
                const float z = z0 * w0 + z1 * w1 + z2 * w2;
                const float dz = std::abs(z - _image.getValue( _image.getIndex(x, y) ) );
                if (dz > maxError) {
                    maxError = dz;
                    maxPoint = glm::ivec2(x, y);
                }
            } else if (wasInside) {
                break;
            }

            w0 += a12;
            w1 += a20;
            w2 += a01;
        }

        w00 += b12;
        w01 += b20;
        w02 += b01;
    }

    if (maxPoint == p0 || maxPoint == p1 || maxPoint == p2) {
        maxError = 0;
    }

    return std::make_pair(maxPoint, maxError);
}

struct TriangulatorData {
    std::vector<glm::ivec2>  points;
    std::vector<int>        triangles;
    std::vector<int>        halfedges;

    std::vector<glm::ivec2>  candidates;
    std::vector<float>      errors;
    std::vector<int>        queueIndexes;
    std::vector<int>        queue;
    std::vector<int>        pending;

    void QueueSwap(const int i, const int j) {
        const int pi = queue[i];
        const int pj = queue[j];
        queue[i] = pj;
        queue[j] = pi;
        queueIndexes[pi] = j;
        queueIndexes[pj] = i;
    };

    bool QueueLess(const int i, const int j) {
        return -errors[queue[i]] < -errors[queue[j]];
    };

    bool QueueDown(const int i0, const int n) {
        int i = i0;
        while (1) {
            const int j1 = 2 * i + 1;
            if (j1 >= n || j1 < 0) {
                break;
            }
            const int j2 = j1 + 1;
            int j = j1;
            if (j2 < n && QueueLess(j2, j1)) {
                j = j2;
            }
            if (!QueueLess(j, i)) {
                break;
            }
            QueueSwap(i, j);
            i = j;
        }
        return i > i0;
    }

    int QueuePopBack() {
        const int t = queue.back();
        queue.pop_back();
        queueIndexes[t] = -1;
        return t;
    }

    int QueuePop() {
        const int n = queue.size() - 1;
        QueueSwap(0, n);
        QueueDown(0, n);
        return QueuePopBack();
    }

    void QueueUp(const int j0) {
        int j = j0;
        while (1) {
            int i = (j - 1) / 2;
            if (i == j || !QueueLess(j, i)) {
                break;
            }
            QueueSwap(i, j);
            j = i;
        }
    }

    void QueuePush(const int t) {
        const int i = queue.size();
        queueIndexes[t] = i;
        queue.push_back(t);
        QueueUp(i);
    }

    void QueueRemove(const int t) {
        const int i = queueIndexes[t];
        if (i < 0) {
            const auto it = std::find(pending.begin(), pending.end(), t);
            if (it != pending.end()) {
                std::swap(*it, pending.back());
                pending.pop_back();
            } 
            else {
                // this shouldn't happen!
            }
            return;
        }
        const int n = queue.size() - 1;
        if (n != i) {
            QueueSwap(i, n);
            if (!QueueDown(i, n)) {
                QueueUp(i);
            }
        }
        QueuePopBack();
    }

    int AddTriangle(const int a, const int b, const int c,
                    const int ab, const int bc, const int ca,
                    int e) {
        if (e < 0) {
            // new halfedge index
            e = triangles.size();
            // add triangle vertices
            triangles.push_back(a);
            triangles.push_back(b);
            triangles.push_back(c);
            // add triangle halfedges
            halfedges.push_back(ab);
            halfedges.push_back(bc);
            halfedges.push_back(ca);
            // add triangle metadata
            candidates.emplace_back(0);
            errors.push_back(0);
            queueIndexes.push_back(-1);
        } 
        else {
            // set triangle vertices
            triangles[e + 0] = a;
            triangles[e + 1] = b;
            triangles[e + 2] = c;
            // set triangle halfedges
            halfedges[e + 0] = ab;
            halfedges[e + 1] = bc;
            halfedges[e + 2] = ca;
        }

        // link neighboring halfedges
        if (ab >= 0)
            halfedges[ab] = e + 0;
        if (bc >= 0)
            halfedges[bc] = e + 1;
        if (ca >= 0)
            halfedges[ca] = e + 2;

        // add triangle to pending queue for later rasterization
        const int t = e / 3;
        pending.push_back(t);

        // return first halfedge index
        return e;
    }

    int AddPoint(const glm::ivec2& _point) {
        const int i = points.size();
        points.push_back(_point);
        return i;
    }

    void Flush( const Image& _image) {
        for (const int t : pending) {

            // rasterize triangle to find maximum pixel error
            const auto pair = FindCandidate(_image,
                                            points[ triangles[t*3+0] ],
                                            points[ triangles[t*3+1] ],
                                            points[ triangles[t*3+2] ]);

            // update metadata
            candidates[t] = pair.first;
            errors[t] = pair.second;

            // add triangle to priority queue
            QueuePush(t);
        }

        pending.clear();
    };

    void Legalize(const int a) {
        
        // if the pair of triangles doesn't satisfy the Delaunay condition
        // (p1 is inside the circumcircle of [p0, pl, pr]), flip them,
        // then do the same check/flip recursively for the new pair of triangles
        //
        //           pl                    pl
        //          /||\                  /  \
        //       al/ || \bl            al/    \a
        //        /  ||  \              /      \
        //       /  a||b  \    flip    /___ar___\
        //     p0\   ||   /p1   =>   p0\---bl---/p1
        //        \  ||  /              \      /
        //       ar\ || /br             b\    /br
        //          \||/                  \  /
        //           pr                    pr

        const auto inCircle = [](
            const glm::ivec2 a, const glm::ivec2 b, const glm::ivec2 c,
            const glm::ivec2 p)
        {
            const int64_t dx = a.x - p.x;
            const int64_t dy = a.y - p.y;
            const int64_t ex = b.x - p.x;
            const int64_t ey = b.y - p.y;
            const int64_t fx = c.x - p.x;
            const int64_t fy = c.y - p.y;
            const int64_t ap = dx * dx + dy * dy;
            const int64_t bp = ex * ex + ey * ey;
            const int64_t cp = fx * fx + fy * fy;
            return dx*(ey*cp-bp*fy)-dy*(ex*cp-bp*fx)+ap*(ex*fy-ey*fx) < 0;
        };

        const int b = halfedges[a];

        if (b < 0) {
            return;
        }

        const int a0 = a - a % 3;
        const int b0 = b - b % 3;
        const int al = a0 + (a + 1) % 3;
        const int ar = a0 + (a + 2) % 3;
        const int bl = b0 + (b + 2) % 3;
        const int br = b0 + (b + 1) % 3;
        const int p0 = triangles[ar];
        const int pr = triangles[a];
        const int pl = triangles[al];
        const int p1 = triangles[bl];

        if (!inCircle(points[p0], points[pr], points[pl], points[p1])) {
            return;
        }

        const int hal = halfedges[al];
        const int har = halfedges[ar];
        const int hbl = halfedges[bl];
        const int hbr = halfedges[br];

        QueueRemove(a / 3);
        QueueRemove(b / 3);

        const int t0 = AddTriangle(p0, p1, pl, -1, hbl, hal, a0);
        const int t1 = AddTriangle(p1, p0, pr, t0, har, hbr, b0);

        Legalize(t0 + 1);
        Legalize(t1 + 2);
    }
};

Mesh toTerrain( const Image& _image,
                const float _zScale,
                const float _maxError, const float _baseHeight, 
                const int _maxTriangles, const int _maxPoints) {

    if (_image.getChannels() != 1)
        return Mesh();

    TriangulatorData data;

    // add points at all four corners
    const int x0 = 0;
    const int y0 = 0;
    const int x1 = _image.getWidth() - 1;
    const int y1 = _image.getHeight() - 1;
    const int p0 = data.AddPoint(glm::ivec2(x0, y0));
    const int p1 = data.AddPoint(glm::ivec2(x1, y0));
    const int p2 = data.AddPoint(glm::ivec2(x0, y1));
    const int p3 = data.AddPoint(glm::ivec2(x1, y1));

    // add initial two triangles
    const int t0 = data.AddTriangle(p3, p0, p2, -1, -1, -1, -1);
    data.AddTriangle(p0, p3, p1, t0, -1, -1, -1);
    data.Flush(_image);

    // helper function to check if triangulation is complete
    const auto done = [&]() {
        const float e = data.errors[data.queue[0]];
        if (e <= _maxError) {
            return true;
        }
        if (_maxTriangles > 0 && data.queue.size() >= _maxTriangles) {
            return true;
        }
        if (_maxPoints > 0 && data.points.size() >= _maxPoints) {
            return true;
        }
        return e == 0;
    };

    while (!done()) {
        // pop triangle with highest error from priority queue
        const int t = data.QueuePop();

        const int e0 = t * 3 + 0;
        const int e1 = t * 3 + 1;
        const int e2 = t * 3 + 2;

        const int p0 = data.triangles[e0];
        const int p1 = data.triangles[e1];
        const int p2 = data.triangles[e2];

        const glm::ivec2 a = data.points[p0];
        const glm::ivec2 b = data.points[p1];
        const glm::ivec2 c = data.points[p2];
        const glm::ivec2 p = data.candidates[t];

        const int pn = data.AddPoint(p);

        const auto collinear = []( const glm::ivec2 p0, const glm::ivec2 p1, const glm::ivec2 p2) {
            return (p1.y-p0.y)*(p2.x-p1.x) == (p2.y-p1.y)*(p1.x-p0.x);
        };

        const auto handleCollinear = [&](const int pn, const int a) {
            const int a0 = a - a % 3;
            const int al = a0 + (a + 1) % 3;
            const int ar = a0 + (a + 2) % 3;
            const int p0 = data.triangles[ar];
            const int pr = data.triangles[a];
            const int pl = data.triangles[al];
            const int hal = data.halfedges[al];
            const int har = data.halfedges[ar];

            const int b = data.halfedges[a];

            if (b < 0) {
                const int t0 = data.AddTriangle(pn, p0, pr, -1, har, -1, a0);
                const int t1 = data.AddTriangle(p0, pn, pl, t0, -1, hal, -1);
                data.Legalize(t0 + 1);
                data.Legalize(t1 + 2);
                return;
            }

            const int b0 = b - b % 3;
            const int bl = b0 + (b + 2) % 3;
            const int br = b0 + (b + 1) % 3;
            const int p1 = data.triangles[bl];
            const int hbl = data.halfedges[bl];
            const int hbr = data.halfedges[br];

            data.QueueRemove(b / 3);

            const int t0 = data.AddTriangle(p0, pr, pn, har, -1, -1, a0);
            const int t1 = data.AddTriangle(pr, p1, pn, hbr, -1, t0 + 1, b0);
            const int t2 = data.AddTriangle(p1, pl, pn, hbl, -1, t1 + 1, -1);
            const int t3 = data.AddTriangle(pl, p0, pn, hal, t0 + 2, t2 + 1, -1);

            data.Legalize(t0);
            data.Legalize(t1);
            data.Legalize(t2);
            data.Legalize(t3);
        };

        if (collinear(a, b, p))
            handleCollinear(pn, e0);
        else if (collinear(b, c, p))
            handleCollinear(pn, e1);
        else if (collinear(c, a, p))
            handleCollinear(pn, e2);
        else {
            const int h0 = data.halfedges[e0];
            const int h1 = data.halfedges[e1];
            const int h2 = data.halfedges[e2];

            const int t0 = data.AddTriangle(p0, p1, pn, h0, -1, -1, e0);
            const int t1 = data.AddTriangle(p1, p2, pn, h1, -1, t0 + 1, -1);
            const int t2 = data.AddTriangle(p2, p0, pn, h2, t0 + 2, t1 + 1, -1);

            data.Legalize(t0);
            data.Legalize(t1);
            data.Legalize(t2);
        }

        data.Flush(_image);
    }

    std::vector<glm::vec2> texcoords;
    std::vector<glm::vec3> points;
    points.reserve(data.points.size());
    texcoords.reserve(data.points.size());
    const int w = _image.getWidth();
    const int h = _image.getHeight();
    const int w1 = w - 1;
    const int h1 = h - 1;

    for (const glm::ivec2 &p : data.points) {
        points.emplace_back(p.x, h1 - p.y, _image.getValue( _image.getIndex(p.x, p.y) ) * _zScale);
        texcoords.emplace_back(p.x/float(w1), 1.0f-p.y/float(h1));
    }

    std::vector<glm::ivec3> triangles;
    triangles.reserve(data.queue.size());
    for (const int i : data.queue) {
        triangles.emplace_back(
            data.triangles[i * 3 + 0],
            data.triangles[i * 3 + 1],
            data.triangles[i * 3 + 2] );
    }

    // BASE
    //
    
    if ( _baseHeight > 0.0f ) {
        const float z = -_baseHeight;// * _zScale;
        
        std::map<int, float> x0s;
        std::map<int, float> x1s;
        std::map<int, float> y0s;
        std::map<int, float> y1s;
        std::unordered_map<glm::vec3, int> lookup;

        // find points along each edge
        for (int i = 0; i < points.size(); i++) {
            const auto &p = points[i];
            bool edge = false;

            if (p.x == 0) {
                x0s[p.y] = p.z;
                edge = true;
            }
            else if (p.x == w1) {
                x1s[p.y] = p.z;
                edge = true;
            }

            if (p.y == 0) {
                y0s[p.x] = p.z;
                edge = true;
            }
            else if (p.y == h1) {
                y1s[p.x] = p.z;
                edge = true;
            }

            if (edge)
                lookup[p] = i;
        }

        std::vector<std::pair<int, float>> sx0s(x0s.begin(), x0s.end());
        std::vector<std::pair<int, float>> sx1s(x1s.begin(), x1s.end());
        std::vector<std::pair<int, float>> sy0s(y0s.begin(), y0s.end());
        std::vector<std::pair<int, float>> sy1s(y1s.begin(), y1s.end());

        const auto pointIndex = [&lookup, &points, &texcoords, &w1, &h1](
            const float x, const float y, const float z)
        {
            const glm::vec3 point(x, y, z);
            if (lookup.find(point) == lookup.end()) {
                lookup[point] = points.size();
                points.push_back(point);
                texcoords.push_back( glm::vec2(x/float(w1), y/float(h1)) );
            }
            return lookup[point];
        };

        // compute base center point
        const int center = pointIndex(w * 0.5f, h * 0.5f, z);

        // edge x = 0
        for (int i = 1; i < sx0s.size(); i++) {
            const int y0 = sx0s[i-1].first;
            const int y1 = sx0s[i].first;
            const float z0 = sx0s[i-1].second;
            const float z1 = sx0s[i].second;
            const int p00 = pointIndex(0, y0, z);
            const int p01 = pointIndex(0, y0, z0);
            const int p10 = pointIndex(0, y1, z);
            const int p11 = pointIndex(0, y1, z1);
            triangles.emplace_back(p01, p10, p00);
            triangles.emplace_back(p01, p11, p10);
            triangles.emplace_back(center, p00, p10);
        }

        // edge x = w1
        for (int i = 1; i < sx1s.size(); i++) {
            const int y0 = sx1s[i-1].first;
            const int y1 = sx1s[i].first;
            const float z0 = sx1s[i-1].second;
            const float z1 = sx1s[i].second;
            const int p00 = pointIndex(w1, y0, z);
            const int p01 = pointIndex(w1, y0, z0);
            const int p10 = pointIndex(w1, y1, z);
            const int p11 = pointIndex(w1, y1, z1);
            triangles.emplace_back(p00, p10, p01);
            triangles.emplace_back(p10, p11, p01);
            triangles.emplace_back(center, p10, p00);
        }

        // edge y = 0
        for (int i = 1; i < sy0s.size(); i++) {
            const int x0 = sy0s[i-1].first;
            const int x1 = sy0s[i].first;
            const float z0 = sy0s[i-1].second;
            const float z1 = sy0s[i].second;
            const int p00 = pointIndex(x0, 0, z);
            const int p01 = pointIndex(x0, 0, z0);
            const int p10 = pointIndex(x1, 0, z);
            const int p11 = pointIndex(x1, 0, z1);
            triangles.emplace_back(p00, p10, p01);
            triangles.emplace_back(p10, p11, p01);
            triangles.emplace_back(center, p10, p00);
        }

        // edge y = h1
        for (int i = 1; i < sy1s.size(); i++) {
            const int x0 = sy1s[i-1].first;
            const int x1 = sy1s[i].first;
            const float z0 = sy1s[i-1].second;
            const float z1 = sy1s[i].second;
            const int p00 = pointIndex(x0, h1, z);
            const int p01 = pointIndex(x0, h1, z0);
            const int p10 = pointIndex(x1, h1, z);
            const int p11 = pointIndex(x1, h1, z1);
            triangles.emplace_back(p01, p10, p00);
            triangles.emplace_back(p01, p11, p10);
            triangles.emplace_back(center, p00, p10);
        }
    }

    Mesh mesh;

    for (const glm::vec3 &p : points)
        mesh.addVertex( p );

    for (const glm::vec2 &t : texcoords)
        mesh.addTexCoord( t );
    
    for (const glm::ivec3 &tri : triangles)
        mesh.addTriangleIndices( tri[0], tri[1], tri[2] );

    return mesh;
}


}
