#include "vera/ops/draw.h"

#include "vera/ops/meshes.h"
#include "vera/ops/fs.h"
#include "vera/ops/string.h"
#include "vera/xr/holoPlay.h"
#include "vera/window.h"

#include "vera/shaders/defaultShaders.h"

#include "glm/gtc/matrix_transform.hpp"

#include <stack>

#ifndef TWO_PI
#define TWO_PI   6.28318530717958647693
#endif

namespace vera {

float       pd              = getDisplayPixelRatio(); // pixel density

Shader*     shaderPtr       = nullptr;
bool        shaderChange    = true; 

HorizontalAlign shapeHAlign = ALIGN_CENTER;
VerticalAlign   shapeVAlign = ALIGN_MIDDLE;

glm::vec4   background_color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
bool        background_enabled = true;

glm::vec4   fill_color      = glm::vec4(1.0f);
Shader*     fill_shader     = nullptr;
bool        fill_enabled    = true;

glm::vec4   stroke_color    = glm::vec4(1.0f);
float       stroke_weight   = 1.0f;
bool        stroke_enabled  = true;
Shader*     stroke_shader   = nullptr;
Shader*     spline_2d_shader   = nullptr;
Shader*     spline_3d_shader   = nullptr;

// POINTS
float       points_size     = 10.0f;
int         points_shape    = 0.0;
Shader*     points_shader   = nullptr;

// 2D PRIMITIVES
Shader*     billboard_shader = nullptr;
Vbo*        billboard_vbo    = new Vbo( rectMesh(0.0,0.0,1.0,1.0) );

std::vector<glm::vec2> cached_circle_coorners;

// 3D PRIMITIVES
Vbo*        plane_vbo        = nullptr;
float       plane_width      = 0.0f;
float       plane_height     = 0.0f;
int         plane_resX       = 0;
int         plane_resY       = 0;
Vbo*        box_vbo          = nullptr;
float       box_width        = 0.0f;
float       box_height       = 0.0f;
float       box_depth        = 0.0f;
Vbo*        sphere_vbo       = nullptr;
int         sphere_resolution = 0;
float       sphere_radius    = 0.0f;

// 3D Scene
Scene*      main_scene           = new Scene();

bool        lights_enabled  = false;

glm::mat4   matrix_world    = glm::mat4(1.0f);
std::stack<glm::mat4> matrix_stack;

//   
//
void print(const std::string& _text) { std::cout << _text << std::endl; }
void frameRate(int _fps) { setFps(_fps); }

void flagChange() { main_scene->flagChange(); }
bool haveChanged()  { return main_scene->haveChange(); }
void resetChange() { main_scene->resetChange(); }

float displayDensity() { return getDisplayPixelRatio(); }
float pixelDensity() { return pd; }
void pixelDensity(float _density) { pd = _density; }

bool fullscreen() { return isFullscreen(); }
void fullscreen(bool _fullscreen) { setFullscreen(_fullscreen); }

void resetMatrix() { matrix_world = glm::mat4(1.0f); }
void applyMatrix(const glm::mat3& _mat ) { matrix_world *= glm::mat4(_mat); }
void applyMatrix(const glm::mat4& _mat ) { matrix_world *= glm::mat4(_mat); };

void rotate(float _rad) { matrix_world = glm::rotate(matrix_world, _rad, glm::vec3(0.0f, 0.0f, 1.0f) ); }
void rotateX(float _rad) { matrix_world = glm::rotate(matrix_world, _rad, glm::vec3(1.0f, 0.0f, 0.0f) ); }
void rotateY(float _rad) { matrix_world = glm::rotate(matrix_world, _rad, glm::vec3(0.0f, 1.0f, 0.0f) ); }
void rotateZ(float _rad) { matrix_world = glm::rotate(matrix_world, _rad, glm::vec3(0.0f, 0.0f, 1.0f) ); }

void scale(float _s) { matrix_world = glm::scale(matrix_world, glm::vec3(_s) ); }
void scale(float _x, float _y, float _z) { matrix_world = glm::scale(matrix_world, glm::vec3(_x, _y, _z) ); }
void scale(const glm::vec2& _s) { matrix_world = glm::scale(matrix_world, glm::vec3(_s, 1.0f) ) ; }
void scale(const glm::vec3& _s) { matrix_world = glm::scale(matrix_world, _s) ; }

void translate(float _x, float _y, float _z) { matrix_world = glm::translate(matrix_world, glm::vec3(_x, _y, _z) ); }
void translate(const glm::vec2& _t) { matrix_world = glm::translate(matrix_world, glm::vec3(_t, 0.0f) ); }
void translate(const glm::vec3& _t) { matrix_world = glm::translate(matrix_world, _t ); }

void push() { matrix_stack.push(matrix_world); }
void pop() { 
    matrix_world = matrix_stack.top(); 
    matrix_stack.pop();
}

// CAMERA
// 
Camera* createCamera(const std::string& _name) {
    Camera* cam;
    CamerasMap::iterator it = main_scene->cameras.find(_name);
    if (it == main_scene->cameras.end()) {
        cam = new Camera();
        main_scene->cameras[_name] = cam;
    }
    else 
        cam = it->second;

    cam->setViewport(getWindowWidth(), getWindowHeight());
    setDepthTest(true);
    return cam;
}

void setCamera(const std::string& _name) {
    CamerasMap::iterator it = main_scene->cameras.find(_name);
    if (it != main_scene->cameras.end()) {
        setCamera(it->second);
    }
    else {
        std::cerr << "Camera '" << _name << "' not found!" << std::endl;
    }
}

void setCamera(Camera& _camera) { setCamera(&_camera); }
void setCamera(Camera* _camera) {
    main_scene->activeCamera = _camera;

    if (main_scene->activeCamera)
        main_scene->activeCamera->begin();
};

void resetCamera() {
    if (main_scene->activeCamera) {
        main_scene->activeCamera->end();
        main_scene->lastCamera = main_scene->activeCamera;
    }

    main_scene->activeCamera = nullptr;
    setDepthTest(false);
};

void addCamera(const std::string& _name, Camera& _camera) { addCamera(_name, &_camera); }
void addCamera(const std::string& _name, Camera* _camera) { main_scene->cameras[_name] = _camera; }

void perspective(float _fovy, float _aspect, float _near, float _far) {
    Camera* cam;

    CamerasMap::iterator it = main_scene->cameras.find("perspective");
    if (it != main_scene->cameras.end()) {
        cam = it->second;
    }
    else {
        cam = new Camera();
        main_scene->cameras["perspective"] = cam;
        cam->setProjection( PERSPECTIVE );
    }
    
    cam->setAspect(_aspect);
    cam->setFOV(_fovy);
    cam->setClipping(_near, _far);
    main_scene->activeCamera = cam;
}

void ortho(float _left, float _right, float _bottom, float _top,  float _near, float _far) {
    Camera* cam;

    CamerasMap::iterator it = main_scene->cameras.find("ortho");
    if (it != main_scene->cameras.end()) {
        cam = it->second;
    }
    else {
        cam = new Camera();
        main_scene->cameras["ortho"] = cam;
        cam->setProjection( ORTHO );
    }

    cam->setProjection( glm::ortho(  _left , _right, _bottom, _top, _near, _top) );
    main_scene->activeCamera = cam;
}

glm::mat4 projectionViewWorldMatrix() {
    if (main_scene->activeCamera)
        return main_scene->activeCamera->getProjectionViewMatrix() * matrix_world; 
    else
        return getFlippedOrthoMatrix() * matrix_world;
}

const glm::mat4& projectionViewMatrix() {
    if (main_scene->activeCamera)
        return main_scene->activeCamera->getProjectionViewMatrix(); 
    else
        return getFlippedOrthoMatrix();
}

const glm::mat4& projectionMatrix() {
    if (main_scene->activeCamera)
        return main_scene->activeCamera->getProjectionMatrix(); 
    else
        return getFlippedOrthoMatrix();
}

const glm::mat4& viewMatrix() {
    if (main_scene->activeCamera)
        return main_scene->activeCamera->getViewMatrix(); 
    else
        return getFlippedOrthoMatrix();
}

const glm::mat4& worldMatrix() { return matrix_world; }
glm::mat4* worldMatrixPtr() { return &matrix_world; }

Shader* pointShader() {
    if (points_shader == nullptr) {
        points_shader = new Shader();
        points_shader->setSource( getDefaultSrc(FRAG_POINTS), getDefaultSrc(VERT_POINTS) );
        addShader("points", points_shader);
    }

    return points_shader;
}

Shader* strokeShader() {
    if (stroke_shader == nullptr) {
        stroke_shader = new Shader();
        stroke_shader->setSource( getDefaultSrc(FRAG_STROKE), getDefaultSrc(VERT_STROKE) );
        addShader("stroke", stroke_shader);
    }
    
    return stroke_shader;
}

Shader* spline2DShader() {
    if (spline_2d_shader == nullptr) {
        spline_2d_shader = new Shader();
        spline_2d_shader->setSource( getDefaultSrc(FRAG_SPLINE_2D), getDefaultSrc(VERT_SPLINE_2D) );
        addShader("spline_2d", spline_2d_shader);
    }
    
    return spline_2d_shader;
}

Shader* spline3DShader() {
    if (spline_3d_shader == nullptr) {
        spline_3d_shader = new Shader();
        spline_3d_shader->setSource( getDefaultSrc(FRAG_SPLINE_3D), getDefaultSrc(VERT_SPLINE_3D) );
        addShader("spline_3d", spline_3d_shader);
    }
    
    return spline_3d_shader;
}

Shader* fillShader() {
    if (fill_shader == nullptr) {
        fill_shader = new Shader();
        fill_shader->setSource( getDefaultSrc(FRAG_FILL), getDefaultSrc(VERT_FILL) );
        addShader("fill", fill_shader);
    }
    
    return fill_shader;
}

void background() { clear(background_color); }
void background( int _brightness ) {
    background( glm::vec4(_brightness / 255.0f, _brightness / 255.0f, _brightness / 255.0f, 1.0f) );
}
void background( int _red, int _green, int _blue) {
    background( glm::vec4(_red / 255.0f, _green / 255.0f, _blue / 255.0f, 1.0f) );
}

void background( int _red, int _green, int _blue, int _alpha) {
    background( glm::vec4(_red / 255.0f, _green / 255.0f, _blue / 255.0f, _alpha / 255.0f) );
}
void background( float _brightness ) { background( glm::vec4(_brightness, _brightness, _brightness, 1.0f) ); }
void background( float _red, float _green, float _blue) { background( glm::vec4(_red, _green, _blue, 1.0f) ); }
void background( float _red, float _green, float _blue, float _alpha) { background( glm::vec4(_red, _green, _blue, _alpha) ); }
void background( const glm::vec3& _color ) { background( glm::vec4(_color, 1.0f) ); }
void background( const glm::vec4& _color ) {
    background_enabled = true;
    clear(_color);
}

bool getBackgroundEnabled() { return background_enabled; }
glm::vec4 getBackground() { return background_color; }

void clear( float _brightness ) { clear( glm::vec4(_brightness, _brightness, _brightness, 1.0f) ); }
void clear( const glm::vec3& _color ) { clear( glm::vec4(_color, 1.0f) ); }
void clear( const glm::vec4& _color ) {
    background_color = _color;
    glClearColor(_color.r, _color.g, _color.b, _color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    // if (main_scene->activeCamera != nullptr || getDepthTest())
    //     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    // else
    //     glClear(GL_COLOR_BUFFER_BIT );
}

const glm::vec4& getFillColor() { return fill_color; }
void noFill() { fill_enabled = false; }
void fill( float _brightness ) { fill( glm::vec4( _brightness, _brightness, _brightness, fill_color.a ) ); }
void fill( float _red, float _green, float _blue) { fill( glm::vec4( _red, _green, _blue, fill_color.a ) );  }
void fill( float _red, float _green, float _blue, float _alpha) { fill( glm::vec4( _red, _green, _blue, _alpha ) );  }
void fill( const glm::vec3& _color ) { fill( glm::vec4( _color, fill_color.a ) ); }
void fill( const glm::vec4& _color ) { 
    fill_color = _color;
    fill_enabled = true;

    if (shaderPtr == nullptr || shaderPtr != fill_shader)
        shaderPtr = fillShader();
}

const glm::vec4& getStrokeColor() { return stroke_color; }
void noStroke() { stroke_enabled = false;}
void stroke( float _brightness ) { stroke( glm::vec4( _brightness, _brightness, _brightness, stroke_color.a ) ); }
void stroke( float _red, float _green, float _blue) { stroke( glm::vec4( _red, _green, _blue, stroke_color.a ) );  }
void stroke( float _red, float _green, float _blue, float _alpha) { stroke( glm::vec4( _red, _green, _blue, _alpha ) );  }
void stroke( const glm::vec3& _color ) { stroke( glm::vec4( _color, stroke_color.a ) ); }
void stroke( const glm::vec4& _color ) { 
    stroke_color = _color;
    stroke_enabled = true;

    if (shaderPtr == nullptr || shaderPtr != fill_shader)
        shaderPtr = fillShader();
}
void strokeWeight( float _weight) {
    stroke_weight = _weight * pd;
}

void pointSize( float _size ) { points_size = _size * pd; }
void pointShape( PointShape _shape) { points_shape = _shape; }


// POINTS
//
void points(const std::vector<glm::vec2>& _positions, Shader* _program) {
    if (_program == nullptr)
        _program = pointShader();
    
    shader(_program);

#if defined(__EMSCRIPTEN__)
    Vbo vbo = _positions;
    vbo.setDrawMode(POINTS);
    vbo.render(_program);
#else

    #if !defined(PLATFORM_RPI) && !defined(DRIVER_DRM) && !defined(_WIN32) && !defined(__EMSCRIPTEN__)
    glEnable(GL_POINT_SPRITE);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    #endif

    const GLint location = _program->getAttribLocation("a_position");
    if (location != -1) {
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, 2, GL_FLOAT, false, 0,  &_positions[0].x);
        glDrawArrays(GL_POINTS, 0, _positions.size());
        glDisableVertexAttribArray(location);
    }
#endif
}

void points(const std::vector<glm::vec3>& _positions, Shader* _program) {
    if (_program == nullptr) 
        _program = pointShader();
    
    shader(_program);

#if defined(__EMSCRIPTEN__)
    Vbo vbo = _positions;
    vbo.setDrawMode(POINTS);
    vbo.render(_program);
#else
    #if !defined(PLATFORM_RPI) && !defined(DRIVER_DRM) && !defined(_WIN32)
    glEnable(GL_POINT_SPRITE);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    #endif
    const GLint location = _program->getAttribLocation("a_position");
    if (location != -1) {
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, 3, GL_FLOAT, false, 0,  &_positions[0].x);
        glDrawArrays(GL_POINTS, 0, _positions.size());
        glDisableVertexAttribArray(location);
    }
#endif
}

void points(const Line& _line, Shader* _program) {
    std::vector<glm::vec3> positions;
    positions.push_back( _line.getPoint(0) );
    positions.push_back( _line.getPoint(1) );
    points(positions, _program);
}

void points(const Triangle& _triangle, Shader* _program) {
    std::vector<glm::vec3> positions;
    positions.push_back( _triangle.getVertex(0) );
    positions.push_back( _triangle.getVertex(1) );
    positions.push_back( _triangle.getVertex(2) );
    points(positions, _program);
}

void points(const BoundingBox& _bbox, Shader* _program) {
    std::vector<glm::vec3> positions;
    positions.push_back( _bbox.min );
    positions.push_back( _bbox.max );
    positions.push_back( glm::vec3(_bbox.min.x, _bbox.min.y, _bbox.max.z) );
    positions.push_back( glm::vec3(_bbox.min.x, _bbox.max.y, _bbox.max.z) );
    positions.push_back( glm::vec3(_bbox.max.x, _bbox.max.y, _bbox.min.z) );
    positions.push_back( glm::vec3(_bbox.max.x, _bbox.min.y, _bbox.min.z) );
    positions.push_back( glm::vec3(_bbox.min.x, _bbox.max.y, _bbox.min.z) );
    positions.push_back( glm::vec3(_bbox.max.x, _bbox.min.y, _bbox.max.z) );
    points(positions, _program);
}

void pointsBoundingBox(const glm::vec4& _bbox, Shader* _program) {
    std::vector<glm::vec2> positions;
    positions.push_back( glm::vec2(_bbox.x, _bbox.y) );
    positions.push_back( glm::vec2(_bbox.z, _bbox.y) );
    positions.push_back( glm::vec2(_bbox.z, _bbox.w) );
    positions.push_back( glm::vec2(_bbox.x, _bbox.w) );
    points(positions, _program);
}

// LINES
// 
void line(float _x1, float _y1, float _x2, float _y2, Shader* _program) {
    std::vector<glm::vec2> pts = {glm::vec2(_x1,_y1), glm::vec2(_x2, _y2)};
    line(pts, _program);
}

void line(float _x1, float _y1, float _z1, float _x2, float _y2, float _z2, Shader* _program) {
    std::vector<glm::vec3> pts = {glm::vec3(_x1,_y1, _z1), glm::vec3(_x2, _y2, _z2) };
    line(pts, _program);
}

void line(const glm::vec2& _a, const glm::vec2& _b, Shader* _program) {
    std::vector<glm::vec2> pts = {_a, _b};
    line(pts, _program);
}

void line(const std::vector<glm::vec2>& _positions, Shader* _program) {
    
    if (stroke_weight > 0.0f && stroke_weight <= 1.0f) {
        if (_program == nullptr)
            _program = strokeShader();
       
        shader(_program);
        _program->setUniform("u_color", stroke_color);
        _program->setUniform("u_strokeWeight", stroke_weight);

    #if defined(__EMSCRIPTEN__)
        Vbo vbo = _positions;
        vbo.setDrawMode(LINE_STRIP);
        vbo.render(_program);
    #else
        glLineWidth(stroke_weight);
        const GLint location = _program->getAttribLocation("a_position");
        if (location != -1) {
            glEnableVertexAttribArray(location);
            glVertexAttribPointer(location, 2, GL_FLOAT, false, 0,  _positions.data());
            glDrawArrays(GL_LINE_STRIP, 0, _positions.size());
            glDisableVertexAttribArray(location);
        }
    #endif
    }
    else {
        // Create a mesh for the line
        Mesh mesh = vera::lineMesh(_positions, stroke_weight);
        Vbo vbo = Vbo( mesh );
        if (_program == nullptr)
            _program = spline2DShader();
        model(vbo, _program);
    }
};

void line(const glm::vec3& _a, const glm::vec3& _b, Shader* _program) {
    std::vector<glm::vec3> pts = {_a, _b};
    line(pts, _program);
}

void line(const Polyline& _polyline, Shader* _program) {
    line(_polyline.get3DPoints(), _program);
}

void line(const std::vector<glm::vec3>& _positions, Shader* _program) {
    if (stroke_enabled == false || _positions.size() < 2)
        return;

    if (stroke_weight > 0.0f && stroke_weight <= 1.0f) {
        if (_program == nullptr)
            _program = strokeShader();

        shader(_program);
        _program->setUniform("u_color", stroke_color);
        _program->setUniform("u_strokeWeight", stroke_weight);
        
        #if defined(__EMSCRIPTEN__)
        Vbo vbo = _positions;
        vbo.setDrawMode(LINE_STRIP);
        vbo.render(_program);
        #else
        glLineWidth(stroke_weight);
        const GLint location = _program->getAttribLocation("a_position");
        if (location != -1) {
            glEnableVertexAttribArray(location);
            glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, 0, (const void*)_positions.data());
            glDrawArrays(GL_LINE_STRIP, 0, _positions.size());
            glDisableVertexAttribArray(location);
        }
        #endif
    }
    else {
        Mesh mesh = vera::lineMesh(_positions);
        Vbo vbo = Vbo( mesh );
        if (_program == nullptr)
            _program = spline3DShader();
        model(vbo, _program);
    }
};

void line(const Line& _line, Shader* _program) {
    std::vector<glm::vec3> positions;
    positions.push_back( _line.getPoint(0) );
    positions.push_back( _line.getPoint(1) );
    line(positions, _program);
}

void line(const Triangle& _triangle, Shader* _program) {
    std::vector<glm::vec3> positions;
    positions.push_back( _triangle.getVertex(0) );
    positions.push_back( _triangle.getVertex(1) );
    positions.push_back( _triangle.getVertex(2) );
    positions.push_back( _triangle.getVertex(0) );
    line(positions, _program);
}

void line(const BoundingBox& _bbox, Shader* _program) {
    std::vector<glm::vec3> positions;
    positions.push_back( _bbox.min );
    positions.push_back( glm::vec3( _bbox.min.x, _bbox.min.y, _bbox.max.z ) );

    // TODO:
    //  - finsih this

    line(positions, _program);
}

void lineBoundingBox(const glm::vec4& _bbox, Shader* _program) {
    std::vector<glm::vec2> positions;
    positions.push_back( glm::vec2(_bbox.x, _bbox.y) );
    positions.push_back( glm::vec2(_bbox.z, _bbox.y) );
    positions.push_back( glm::vec2(_bbox.z, _bbox.w) );
    positions.push_back( glm::vec2(_bbox.x, _bbox.w) );
    positions.push_back( glm::vec2(_bbox.x, _bbox.y) );
    line(positions, _program);
}

// 2D PRIMITIVES
void triangle(float _x1, float _y1, float _x2, float _y2, float _x3, float _y3, Shader* _program) {
    triangle(glm::vec2(_x1,_y1), glm::vec2(_x2, _y2), glm::vec2(_x3, _y3), _program);
}
void triangle(const glm::vec2& _a, const glm::vec2& _b, const glm::vec2& _c, Shader* _program) {
    std::vector<glm::vec2> pts = {_a, _b, _c};
    triangles(pts, _program);
}
void triangle(float _x1, float _y1, float _z1, float _x2, float _y2, float _z2, float _x3, float _y3, float _z3, Shader* _program) {
    triangle(glm::vec3(_x1,_y1, _z1), glm::vec3(_x2, _y2, _z2), glm::vec3(_x3, _y3, _z3), _program);
}

void triangle(const glm::vec3& _a, const glm::vec3& _b, const glm::vec3& _c, Shader* _program) {
    std::vector<glm::vec3> pts = {_a, _b, _c};
    triangles(pts, _program);
}

void triangle(const glm::vec2& _center, float angle, float _radius,  Shader* _program) {
    std::vector<glm::vec2> pts;
    for (int i = 0; i < 3; i++) {
        float a = angle + TWO_PI / 3.0f * i;
        pts.push_back( glm::vec2( _center.x + cos(a) * _radius, _center.y + sin(a) * _radius) );
    }
    triangles(pts, _program);
}

void triangle(const glm::vec3& _center, float angle, float _radius,  Shader* _program) {
    std::vector<glm::vec3> pts;
    for (int i = 0; i < 3; i++) {
        float a = angle + TWO_PI / 3.0f * i;
        pts.push_back( glm::vec3( _center.x + cos(a) * _radius, _center.y + sin(a) * _radius, 0.0f ) );
    }
    triangles(pts, _program);
}

void triangle(const glm::vec3& _center, glm::vec3 _up, float _radius, Shader* _program) {
    glm::vec3 a = _center + _up * _radius;
    glm::vec3 b = _center + glm::normalize( glm::cross(_up, glm::vec3(0.0f, 0.0f, 1.0f)) ) * _radius * 0.5f;
    glm::vec3 c = _center + glm::normalize( glm::cross(_up, glm::vec3(0.0f, 0.0f, -1.0f)) ) * _radius * 0.5f;
    std::vector<glm::vec3> pts = {a, b, c};
    triangles(pts, _program);
}

void triangles(const std::vector<glm::vec2>& _positions, Shader* _program) {
    if (_program == nullptr)
        _program = fillShader();

    shader(_program);

#if defined(__EMSCRIPTEN__)
    Vbo vbo = _positions;
    vbo.setDrawMode(TRIANGLES);
    vbo.render(_program);
#else
    const GLint location = _program->getAttribLocation("a_position");
    if (location != -1) {
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, 2, GL_FLOAT, false, 0,  &_positions[0].x);
        glDrawArrays(GL_TRIANGLES, 0, _positions.size());
        glDisableVertexAttribArray(location);
    }
#endif
}

void triangles(const std::vector<glm::vec3>& _positions, Shader* _program) {
    if (_program == nullptr)
        _program = fillShader();

    shader(_program);

#if defined(__EMSCRIPTEN__)
    Vbo vbo = _positions;
    vbo.setDrawMode(TRIANGLES);
    vbo.render(_program);
#else
    const GLint location = _program->getAttribLocation("a_position");
    if (location != -1) {
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, 3, GL_FLOAT, false, 0,  &_positions[0].x);
        glDrawArrays(GL_TRIANGLES, 0, _positions.size());
        glDisableVertexAttribArray(location);
    }
#endif
}

void rectAlign(VerticalAlign _align) { shapeVAlign = _align; }
void rectAlign(HorizontalAlign _align) { shapeHAlign = _align; }
VerticalAlign rectVerticalAlign() { return shapeVAlign; }
HorizontalAlign rectHorizontalAlign() { return shapeHAlign; }

void rect(const glm::vec2& _pos, const glm::vec2& _size, Shader* _program) { rect(_pos.x, _pos.y, _size.x, _size.y, _program); }
void rect(float _x, float _y, float _w, float _h, Shader* _program) {
    if (shapeHAlign == ALIGN_CENTER)
        _x -= _w * 0.5f;
    else if (shapeHAlign == ALIGN_RIGHT)
        _x -= _w;
    
    if (shapeVAlign == ALIGN_MIDDLE)
        _y -= _h * 0.5f;
    else if (shapeVAlign == ALIGN_BOTTOM)
        _y -= _h;

    std::vector<glm::vec2> coorners = { glm::vec2(_x, _y),     glm::vec2(_x + _w, _y), 
                                        glm::vec2(_x + _w, _y + _h), glm::vec2(_x, _y + _h),
                                        glm::vec2(_x, _y) };
        
    if (fill_enabled) {
        std::vector<glm::vec2> tris = {     coorners[0], coorners[1], coorners[2],
                                            coorners[2], coorners[3], coorners[0] };
        triangles(tris, _program);
    }

    if (stroke_enabled)
        line(coorners, _program);
}

void rect(const BoundingBox& _bbox, Shader* _program) {
    if (_program == nullptr)
        _program = fillShader();

    std::vector<glm::vec2> coorners = { glm::vec2(_bbox.min.x, _bbox.min.y), 
                                        glm::vec2(_bbox.max.x, _bbox.min.y), 
                                        glm::vec2(_bbox.max.x, _bbox.max.y), 
                                        glm::vec2(_bbox.min.x, _bbox.max.y),
                                        glm::vec2(_bbox.min.x, _bbox.min.y) };

    if (fill_enabled) {
        std::vector<glm::vec2> tris = {     coorners[0], coorners[1], coorners[2],
                                            coorners[2], coorners[3], coorners[0] };
        triangles(tris, _program);
    }

    if (stroke_enabled)
        line(coorners, _program);
}

// CIRCLES
void circleResolution(int _resolution) { 
    cached_circle_coorners.clear();
    const float angleStep = TWO_PI / (float)_resolution;
    for (int i = 0; i < _resolution; i++) {
        float angle = angleStep * i;
        cached_circle_coorners.push_back( glm::vec2( cos(angle), sin(angle) ) );
    }
}

void circle(float _x, float _y, float _radius, Shader* _program) { circle(glm::vec2(_x, _y), _radius, _program); }
void circle(const glm::vec2& _pos, float _radius, Shader* _program) {
    if (cached_circle_coorners.size() == 0)
        circleResolution();
    
    const int numSegments = cached_circle_coorners.size();
    const float angleStep = TWO_PI / (float)numSegments;
    const float radius = _radius;

    if (fill_enabled) {
        std::vector<glm::vec2> tris;
        for (int i = 0; i < numSegments; i++) {
            tris.push_back( _pos );
            tris.push_back( cached_circle_coorners[i] * radius + _pos );
            tris.push_back( cached_circle_coorners[(i + 1) % numSegments] * radius + _pos );
        }
        triangles(tris, _program);
    }

    if (stroke_enabled) {
        std::vector<glm::vec2> lines;
        for (int i = 0; i < numSegments; i++) {
            lines.push_back( cached_circle_coorners[i] * radius + _pos );
        }
        lines.push_back( lines[0] ); // close the circle
        line(lines, _program);
    }
}

// 3D PRIMITIVES
void plane(Shader* _program) { plane(1.0f, _program); }
void plane(float _size, Shader* _program) { plane(_size, _size, _program); }
void plane(float _width, float _height, Shader* _program) { plane(_width, _height, 1, _program); }
void plane(float _width, float _height, int _res, Shader* _program) { plane(_width, _height, _res, _res, _program); }
void plane(float _width, float _height, int _resX, int _resY, Shader* _program) {
    if (plane_vbo != nullptr && (plane_width != _width || plane_height != _height || plane_resX != _resX || plane_resY != _resY)) {
        delete plane_vbo;
        plane_vbo = nullptr;
    }

    if (plane_vbo == nullptr) {
        plane_width = _width;
        plane_height = _height;
        plane_resX = _resX;
        plane_resY = _resY;
        plane_vbo = new Vbo( planeMesh(_width, _height, _resX, _resY) );
    }

    model(plane_vbo, _program);
}

void box(Shader* _program) { box(1.0f, _program); }
void box(float _size, Shader* _program) { box(_size, _size, _size, _program); }
void box(float _width, float _height, Shader* _program) { box(_width, _height, 1.0f, _program); }
void box(float _width, float _height, float _depth, Shader* _program) {
    if (box_vbo != nullptr && (box_width != _width || box_height != _height || box_depth != _depth)) {
        delete box_vbo;
        box_vbo = nullptr;
    }

    if (box_vbo == nullptr) {
        box_width = _width;
        box_height = _height;
        box_depth = _depth;
        box_vbo = new Vbo( boxMesh(_width, _height, _depth) );
    }

    model(box_vbo, _program);
}

void sphere(Shader* _program) { sphere(1.0f, _program); }
void sphere(float _radius, Shader* _program) { sphere(_radius, 36, _program); }
void sphere(float _radius, int _res, Shader* _program) {
    if (sphere_vbo != nullptr && (sphere_radius != _radius || sphere_resolution != _res)) {
        delete sphere_vbo;
        sphere_vbo = nullptr;
    }

    if (sphere_vbo == nullptr) {
        sphere_radius = _radius;
        sphere_resolution = _res;
        sphere_vbo = new Vbo( sphereMesh(_res, _radius) );
    }

    model(sphere_vbo, _program);
}

// IMAGE
// 
Vbo* billboard() { return billboard_vbo; }

Image loadImage(const std::string& _name) {
    Image img;
    img.load(_name);
    return img;
}

void image(const Image &_img) { image(&_img); }
void image(const Image *_img) {
    std::string name = _img->getFilePath();
    TexturesMap::iterator it = main_scene->textures.find( name );
    if (it == main_scene->textures.end())
        main_scene->textures[ name ] = new Texture();
    main_scene->textures[ name ]->load( _img );
    image( main_scene->textures[ name ] );
}

void image(const Image &_img, float _x, float _y, float _width, float _height) { image(&_img, _x, _y, _width, _height); }
void image(const Image *_img, float _x, float _y, float _width, float _height) {
    std::string name = _img->getFilePath();
    TexturesMap::iterator it = main_scene->textures.find( name );
    if (it == main_scene->textures.end())
        main_scene->textures[ name ] = new Texture();
    main_scene->textures[ name ]->load( _img );
    image( main_scene->textures[ name ], _x, _y, _width, _height );
}

void image(const std::string &_path) {
    TexturesMap::iterator it = main_scene->textures.find( _path );
    if (it == main_scene->textures.end()) {
        Texture* tex = new Texture();
        tex->load( _path );
        main_scene->textures[ _path ] = tex;
    }
    image( main_scene->textures[ _path ] );
}

void image(const std::string &_path, float _x, float _y, float _width, float _height) {
    TexturesMap::iterator it = main_scene->textures.find( _path );
    if (it == main_scene->textures.end()) {
        Texture* tex = new Texture();
        tex->load( _path );
        main_scene->textures[ _path ] = tex;
    }
    image( main_scene->textures[ _path ], _x, _y, _width, _height );
}


void image(const Texture &_tex) { image(&_tex); }
void image(const Texture *_tex) {
    if (!_tex)
        return;

    if (billboard_shader == nullptr) {
        billboard_shader = new Shader();
        billboard_shader->setSource( getDefaultSrc(FRAG_DYNAMIC_BILLBOARD), getDefaultSrc(VERT_DYNAMIC_BILLBOARD) );
    }

    billboard_shader->use();
    billboard_shader->setUniform("u_depth", 0.0f);
    billboard_shader->setUniform("u_scale", 1.0f, 1.0f);
    billboard_shader->setUniform("u_translate", 0.0f, 0.0f);
    billboard_shader->setUniform("u_modelViewProjectionMatrix", glm::mat4(1.0) );
    billboard_shader->setUniformTexture("u_tex0", _tex, 0);
    billboard_vbo->render( billboard_shader );
}

void image(const Texture &_tex, float _x, float _y, float _width, float _height) { image(&_tex, _x, _y, _width, _height); }
void image(const Texture *_tex, float _x, float _y, float _width, float _height) {
    if (!_tex)
        return;

    if (billboard_shader == nullptr) {
        billboard_shader = new Shader();
        billboard_shader->setSource( getDefaultSrc(FRAG_DYNAMIC_BILLBOARD), getDefaultSrc(VERT_DYNAMIC_BILLBOARD) );
    }

    if (_width == 0)
        _width = _tex->getWidth();

    if (_height == 0)
        _height = _tex->getHeight();

    billboard_shader->use();
    billboard_shader->setUniform("u_depth", 0.0f);
    billboard_shader->setUniform("u_translate", _x, _y);
    billboard_shader->setUniform("u_scale", _width, _height);
    billboard_shader->setUniform("u_modelViewProjectionMatrix", vera::getOrthoMatrix() );
    billboard_shader->setUniformTexture("u_tex0", _tex, 0);
    billboard_shader->setUniform("u_tex0CurrentFrame", 0.0f );
    billboard_shader->setUniform("u_tex0TotalFrames", 0.0f );
    billboard_vbo->render( billboard_shader );
}

void image(const TextureStream &_stream) { image(&_stream); }
void image(const TextureStream *_stream) {
    if (billboard_shader == nullptr) {
        billboard_shader = new Shader();
        billboard_shader->setSource( getDefaultSrc(FRAG_DYNAMIC_BILLBOARD), getDefaultSrc(VERT_DYNAMIC_BILLBOARD) );
    }

    billboard_shader->use();
    billboard_shader->setUniform("u_depth", 0.0f);
    billboard_shader->setUniform("u_scale", 1.0f, 1.0f);
    billboard_shader->setUniform("u_translate", 0.0f, 0.0f);
    billboard_shader->setUniform("u_modelViewProjectionMatrix", glm::mat4(1.0) );
    billboard_shader->setUniformTexture("u_tex0", _stream, 0);
    billboard_vbo->render( billboard_shader );
}

void image(const TextureStream &_stream, float _x, float _y, float _width, float _height, bool _debug) { image(&_stream, _x, _y, _width, _height, _debug); }
void image(const TextureStream *_stream, float _x, float _y, float _width, float _height, bool _debug) {
    if (billboard_shader == nullptr) {
        billboard_shader = new Shader();
        billboard_shader->setSource( getDefaultSrc(FRAG_DYNAMIC_BILLBOARD), getDefaultSrc(VERT_DYNAMIC_BILLBOARD) );
    }

    if (_width == 0)
        _width = _stream->getWidth();

    if (_height == 0)
        _height = _stream->getHeight();

    billboard_shader->use();
    billboard_shader->setUniform("u_depth", 0.0f);
    billboard_shader->setUniform("u_translate", _x, _y);
    billboard_shader->setUniform("u_scale", _width, _height);
    billboard_shader->setUniform("u_modelViewProjectionMatrix", vera::getOrthoMatrix() );
    billboard_shader->setUniformTexture("u_tex0", _stream, 0);
    if (_debug) {
        billboard_shader->setUniform("u_tex0CurrentFrame", _stream->getCurrentFrame() );
        billboard_shader->setUniform("u_tex0TotalFrames", _stream->getTotalFrames() );
    }
    else {
        billboard_shader->setUniform("u_tex0CurrentFrame", 0.0f);
        billboard_shader->setUniform("u_tex0TotalFrames", 0.0f);
    }
    billboard_vbo->render( billboard_shader );
}

void image(const Fbo &_fbo) { image(&_fbo); }
void image(const Fbo *_fbo) {
    if (billboard_shader == nullptr) {
        billboard_shader = new Shader();
        billboard_shader->setSource( getDefaultSrc(FRAG_DYNAMIC_BILLBOARD), getDefaultSrc(VERT_DYNAMIC_BILLBOARD) );
    }

    billboard_shader->use();
    billboard_shader->setUniform("u_depth", 0.0f);
    billboard_shader->setUniform("u_scale", 1.0f, 1.0f);
    billboard_shader->setUniform("u_translate", 0.0f, 0.0f);
    billboard_shader->setUniform("u_modelViewProjectionMatrix", glm::mat4(1.0) );
    billboard_shader->setUniformTexture("u_tex0", _fbo, 0);
    billboard_vbo->render( billboard_shader );
}

void image(const Fbo &_fbo, float _x, float _y, float _width, float _height) { image(&_fbo, _x, _y, _width, _height); }
void image(const Fbo *_fbo, float _x, float _y, float _width, float _height) { 
    if (billboard_shader == nullptr) {
        billboard_shader = new Shader();
        billboard_shader->setSource( getDefaultSrc(FRAG_DYNAMIC_BILLBOARD), getDefaultSrc(VERT_DYNAMIC_BILLBOARD) );
    }

    if (_width == 0)
        _width = _fbo->getWidth();

    if (_height == 0)
        _height = _fbo->getHeight();

    billboard_shader->use();
    billboard_shader->setUniform("u_depth", 0.0f);
    billboard_shader->setUniform("u_translate", _x, _y);
    billboard_shader->setUniform("u_scale", _width, _height);
    billboard_shader->setUniform("u_modelViewProjectionMatrix", vera::getOrthoMatrix() );
    billboard_shader->setUniformTexture("u_tex0", _fbo, 0);
    billboard_shader->setUniform("u_tex0CurrentFrame", 0.0f );
    billboard_shader->setUniform("u_tex0TotalFrames", 0.0f );
    billboard_vbo->render( billboard_shader );
}

void imageDepth(const Fbo &_fbo, float _x, float _y, float _width, float _height, float _far, float _near) { imageDepth(&_fbo, _x, _y, _width, _height, _far, _near); }
void imageDepth(const Fbo *_fbo, float _x, float _y, float _width, float _height, float _far, float _near) { 
    if (billboard_shader == nullptr) {
        billboard_shader = new Shader();
        billboard_shader->setSource( getDefaultSrc(FRAG_DYNAMIC_BILLBOARD), getDefaultSrc(VERT_DYNAMIC_BILLBOARD) );
    }

    if (_width == 0)
        _width = _fbo->getWidth();

    if (_height == 0)
        _height = _fbo->getHeight();

    billboard_shader->use();
    billboard_shader->setUniform("u_depth", 1.0f);
    billboard_shader->setUniform("u_translate", _x, _y);
    billboard_shader->setUniform("u_scale", _width, _height);
    billboard_shader->setUniform("u_modelViewProjectionMatrix", vera::getOrthoMatrix() );
    billboard_shader->setUniformDepthTexture("u_tex0", _fbo, 0);
    billboard_shader->setUniform("u_tex0CurrentFrame", 0.0f );
    billboard_shader->setUniform("u_tex0TotalFrames", 0.0f );
    billboard_shader->setUniform("u_cameraNearClip", _near);
    billboard_shader->setUniform("u_cameraFarClip", _far);

    billboard_vbo->render( billboard_shader );
}

// FONT
//
Font* loadFont(const std::string& _file) {
    Font* newFont = new Font();
    newFont->load( _file );
    return newFont;
}

void  addFont(const std::string _name, Font& _font) { addFont(_name, &_font); }
void  addFont(const std::string _name, Font* _font) { main_scene->fonts[_name] = _font; }
void  addFont(const std::string _name, const std::string& _file) { 
    main_scene->addFont(_name, _file);
}

Font* font() { 
    if (!main_scene->activeFont)
        main_scene->activeFont = main_scene->getDefaultFont();

    return main_scene->activeFont;
}

Font* font(const std::string& _name) { 
    FontsMap::iterator it = main_scene->fonts.find(_name);
    if (it != main_scene->fonts.end())
        return it->second;
    return main_scene->activeFont;
}

//  FONT
//
Font* textFont(const std::string& _name) { 
    if (_name == "default")
        main_scene->getDefaultFont();

    FontsMap::iterator it = main_scene->fonts.find(_name);
    if (it != main_scene->fonts.end())
        main_scene->activeFont = it->second;

    return main_scene->activeFont; 
}

Font* textFont(const std::string& _name, float _size) { 
    Font* f = textFont(_name);
    if (f)
        textSize(_size, f);
    return f;
}

void textAlign(HorizontalAlign _align, Font* _font) {
    if (_font == nullptr)
        _font = font();
    _font->setAlign( _align );
}

void textAlign(VerticalAlign _align, Font* _font) {
    if (_font == nullptr)
        _font = font();
    _font->setAlign( _align );
}

void textAngle(float _angle, Font* _font) {
    if (_font == nullptr)
        _font = font();
    _font->setAngle( _angle );
}

void textSize(float _size, Font* _font) { 
    if (_font == nullptr)
        _font = font();

    if (getWindowStyle() == LENTICULAR)
        _font->setSize(_size * 3.0f * pd);
    else
        _font->setSize(_size * pd);
}

float textWidth(const std::string& _text, Font* _font) {
    if (_font == nullptr)
        _font = font();
    return _font->getBoundingBox(_text).z;
}

float textHeight(Font* _font) {
    if (_font == nullptr)
        _font = font();
    return _font->getHeight() * getDisplayPixelRatio();
}

float textHeight(const std::string& _text, Font* _font) {
    if (_font == nullptr)
        _font = font();
    return _font->getBoundingBox(_text).w * getDisplayPixelRatio();
}

BoundingBox textBoundingBox(const std::string& _text, float _x, float _y, Font* _font) { 
    return textBoundingBox(_text, glm::vec2(_x, _y), _font);
}

BoundingBox textBoundingBox(const std::string& _text, const glm::vec2& _pos, Font* _font) {
    if (_font == nullptr)
        _font = font();
    glm::vec4 bbox = _font->getBoundingBox(_text);
    return BoundingBox(_pos.x + bbox.x, _pos.y + bbox.y, bbox.z * getDisplayPixelRatio(), bbox.w * getDisplayPixelRatio());
}

void text(const std::string& _text, const glm::vec2& _pos, Font* _font) { text(_text, _pos.x, _pos.y, _font); }
void text(const std::string& _text, float _x, float _y, Font* _font) {
    if (_font == nullptr)
        _font = font();
    _font->setColor( fill_color );

    glm::vec4 pos = projectionViewWorldMatrix() * glm::vec4(_x, _y, 0.0f, 1.0f);
    pos.x /= pos.w;
    pos.y /= pos.w;
    pos.x = (pos.x + 1.0f) * 0.5f * vera::getViewport().z;
    pos.y = (1.0f-pos.y) * 0.5f * vera::getViewport().w;

    _font->render(_text, pos.x, pos.y);
}

void text(const std::string& _text, const glm::vec3& _pos, Font* _font) { 
    if (_font == nullptr)
        _font = font();

    Camera* cam = main_scene->activeCamera;

    glm::vec3 screenPos = cam->worldToScreen(_pos, worldMatrixPtr());
    screenPos.x *= vera::getViewport().z;
    screenPos.y *= vera::getViewport().w;

    // Is in view? (depth and in viewport)
    if (screenPos.z >= 1.0) {
        return;
    }

    _font->setColor( fill_color );
    _font->render(_text, screenPos.x, screenPos.y);
}

void textHighlight(const std::string& _text, const glm::vec2& _pos, const glm::vec4& _bg, Font* _font) { textHighlight(_text, _pos.x, _pos.y, _bg, _font); }
void textHighlight(const std::string& _text, float _x , float _y, const glm::vec4& _bg, Font* _font) {
    if (_font == nullptr)
        _font = font();

    // record sty
    glm::vec4       style_fg = fill_color;
    bool            style_stroke = stroke_enabled;
    bool            style_fill = fill_enabled;
    HorizontalAlign style_halign = _font->getHorizontalAlign();
    VerticalAlign   style_valign = _font->getVerticalAlign();

    HorizontalAlign style_rect_halign = shapeHAlign;
    VerticalAlign   style_rect_valign = shapeVAlign;

    BoundingBox bbox = textBoundingBox(_text, _x, _y, _font);
    bbox.expand(4.0f);

    fill(_bg);
    noStroke();

    shapeHAlign = ALIGN_CENTER;
    shapeVAlign = ALIGN_MIDDLE;
    rect(bbox);
    // rect(bbox.getCenter().x, bbox.getCenter().y, bbox.getWidth() * pd * 1.05, bbox.getHeight() * pd * 1.25);
    
    // restore style
    fill_enabled = style_fill;
    stroke_enabled = style_stroke;
    _font->setAlign(style_halign);
    _font->setAlign(style_valign);
    // style_rect_halign = shapeHAlign;
    // style_rect_valign = shapeVAlign;
    shapeHAlign = style_rect_halign;
    shapeVAlign = style_rect_valign;

    fill(style_fg);
    text(_text, _x, _y, _font);
}

// SHADER
//
Shader* loadShader(const std::string& _fragFile, const std::string& _vertFile) {
    Shader* s = new Shader();
    s->setSource(loadGlslFrom(_fragFile), loadGlslFrom(_vertFile));
    return s;
}

Shader* createShader(const std::string& _fragSrc, const std::string& _vertSrc) {
    Shader* s = new Shader();
    // s.addDefine("MODEL_VERTEX_COLOR", "v_color");
    // s.addDefine("MODEL_VERTEX_NORMAL", "v_normal");
    // s.addDefine("MODEL_VERTEX_TEXCOORD", "v_texcoord");
    // s.addDefine("MODEL_VERTEX_TANGENT", "v_tangent");

    if (!_fragSrc.empty() && _vertSrc.empty())
        s->setSource(_fragSrc, getDefaultSrc(VERT_DEFAULT_SCENE));
    else if (_fragSrc.empty())
        s->setSource(getDefaultSrc(FRAG_DEFAULT_SCENE), getDefaultSrc(VERT_DEFAULT_SCENE));
    else
        s->setSource(_fragSrc, _vertSrc);

    return s;
}

Shader* createShader(DefaultShaders _frag, DefaultShaders _vert) {
    Shader* s = new Shader();
    // s.addDefine("MODEL_VERTEX_COLOR", "v_color");
    // s.addDefine("MODEL_VERTEX_NORMAL", "v_normal");
    // s.addDefine("MODEL_VERTEX_TEXCOORD", "v_texcoord");
    // s.addDefine("MODEL_VERTEX_TANGENT", "v_tangent");
    s->setSource(getDefaultSrc(_frag), getDefaultSrc(_vert));
    return s;
}

std::vector<std::string> listShaders() {
    std::vector<std::string> names;
    for (ShadersMap::iterator it = main_scene->shaders.begin(); it != main_scene->shaders.end(); ++it) {
        names.push_back(it->first);
    }
    return names;
}

Shader* addShader(const std::string& _name, Shader* _shader) { main_scene->shaders[_name] = _shader; return _shader; }
Shader* addShader(const std::string& _name, Shader& _shader) { return addShader(_name, &_shader); }
Shader* addShader(const std::string& _name, const std::string& _frag, const std::string& _vert) {
    main_scene->shaders[_name] = new Shader();

    std::string vertSrc = _vert;
    if (vertSrc.empty()) {
        vertSrc = getDefaultSrc(VERT_DEFAULT_SCENE);
    }
    else if (vertSrc.find(".vert") != std::string::npos) {
        vertSrc = loadGlslFrom(vertSrc);
    }

    std::string fragSrc = _frag;
    if (fragSrc.empty()) {
        fragSrc = getDefaultSrc(FRAG_DEFAULT_SCENE);
    }
    else if (fragSrc.find(".frag") != std::string::npos) {
        fragSrc = loadGlslFrom(fragSrc);
    }

    main_scene->shaders[_name]->setSource(fragSrc, vertSrc);

    return main_scene->shaders[_name];
}

void resetShader() { shaderPtr = nullptr; }

Shader* shader(const std::string& _name) {
    ShadersMap::iterator it = main_scene->shaders.find(_name);
    if (it != main_scene->shaders.end()) {
        return shader(it->second);
    }
    return nullptr;
}

Shader* shader() { return shaderPtr; }
Shader* shader(Shader& _program) { return shader(&_program); }
Shader* shader(Shader* _program) {
    if (shaderPtr != fill_shader || shaderPtr != points_shader || shaderPtr != stroke_shader || 
        shaderPtr != spline_2d_shader || shaderPtr != spline_3d_shader ) {
        shaderPtr = _program; 
        shaderChange = true;
    }

    _program->textureIndex = 0;

    if (!_program->inUse() && shaderChange) {
        _program->use();
    }

    _program->setUniform("u_date", getDate() );
    _program->setUniform("u_resolution", (float)getWindowWidth(), (float)getWindowHeight() );
    _program->setUniform("u_mouse", getMousePosition() );
    _program->setUniform("u_time", (float)getTimeSec() );
    _program->setUniform("u_delta", (float)getDelta() );
    _program->setUniform("u_pixelDensity", pixelDensity() );

    if (_program == fill_shader)
        _program->setUniform("u_color", fill_color);
    else if (_program == stroke_shader || _program == spline_2d_shader || _program == spline_3d_shader) {
        _program->setUniform("u_color", stroke_color);
        _program->setUniform("u_strokeWeight", stroke_weight);
    }
    else if (_program == points_shader) {
        _program->setUniform("u_size", points_size);
        _program->setUniform("u_shape", points_shape);
        _program->setUniform("u_color", fill_color);
        #if !defined(PLATFORM_RPI) && !defined(DRIVER_DRM) && !defined(_WIN32) && !defined(__EMSCRIPTEN__)
        glEnable(GL_POINT_SPRITE);
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
        #endif
    }

    if (main_scene->activeCamera) {
        _program->setUniform("u_modelViewProjectionMatrix", main_scene->activeCamera->getProjectionViewMatrix() * matrix_world );
        _program->setUniform("u_projectionMatrix", main_scene->activeCamera->getProjectionMatrix());
        _program->setUniform("u_normalMatrix", main_scene->activeCamera->getNormalMatrix());
        _program->setUniform("u_viewMatrix", main_scene->activeCamera->getViewMatrix() );

        _program->setUniform("u_camera", -main_scene->activeCamera->getPosition() );
        _program->setUniform("u_cameraDistance", main_scene->activeCamera->getDistance());
        _program->setUniform("u_cameraNearClip", main_scene->activeCamera->getNearClip());
        _program->setUniform("u_cameraFarClip", main_scene->activeCamera->getFarClip());
        _program->setUniform("u_cameraEv100", main_scene->activeCamera->getEv100());
        _program->setUniform("u_cameraExposure", float(main_scene->activeCamera->getExposure()));
        _program->setUniform("u_cameraAperture", main_scene->activeCamera->getAperture());
        _program->setUniform("u_cameraShutterSpeed", main_scene->activeCamera->getShutterSpeed());
        _program->setUniform("u_cameraSensitivity", main_scene->activeCamera->getSensitivity());
        _program->setUniform("u_cameraChange", main_scene->activeCamera->bChange);
        _program->setUniform("u_iblLuminance", float(30000.0f * main_scene->activeCamera->getExposure()));
    }
    else {
        _program->setUniform("u_modelViewProjectionMatrix", getFlippedOrthoMatrix() * matrix_world );
        _program->setUniform("u_projectionMatrix", getFlippedOrthoMatrix() );
        _program->setUniform("u_viewMatrix", glm::mat4(1.0f) );
        _program->setUniform("u_normalMatrix", glm::mat3(1.0f) );
    }

    _program->setUniform("u_modelMatrix", matrix_world );

    if (lights_enabled) {
        // Pass Light Uniforms
        if (main_scene->lights.size() == 1) {
            LightsMap::iterator it = main_scene->lights.begin();

            _program->setUniform("u_lightColor", it->second->color);
            _program->setUniform("u_lightIntensity", it->second->intensity);

            // if (it->second->getLightType() != vera::LIGHT_DIRECTIONAL)
            _program->setUniform("u_light", it->second->getPosition());
            if (it->second->getLightType() == vera::LIGHT_DIRECTIONAL || it->second->getLightType() == vera::LIGHT_SPOT)
                _program->setUniform("u_lightDirection", it->second->direction);
            if (it->second->falloff > 0)
                _program->setUniform("u_lightFalloff", it->second->falloff);

            _program->setUniform("u_lightMatrix", it->second->getBiasMVPMatrix() );

            if (it->second->getShadowMap()->isAllocated())
                _program->setUniformDepthTexture("u_lightShadowMap", it->second->getShadowMap(), _program->textureIndex++ );
        }
        else {
            for (LightsMap::iterator it = main_scene->lights.begin(); it != main_scene->lights.end(); ++it) {
                std::string name = "u_" + it->first;

                _program->setUniform(name + "Color", it->second->color);
                _program->setUniform(name + "Intensity", it->second->intensity);
                if (it->second->getLightType() != vera::LIGHT_DIRECTIONAL)
                    _program->setUniform(name, it->second->getPosition());
                if (it->second->getLightType() == vera::LIGHT_DIRECTIONAL || it->second->getLightType() == vera::LIGHT_SPOT)
                    _program->setUniform(name + "Direction", it->second->direction);
                if (it->second->falloff > 0)
                    _program->setUniform(name +"Falloff", it->second->falloff);

                _program->setUniform(name + "Matrix", it->second->getBiasMVPMatrix() );

                if (it->second->getShadowMap()->isAllocated())
                    _program->setUniformDepthTexture(name + "ShadowMap", it->second->getShadowMap(), _program->textureIndex++ );
            }
        } 
    }

    return _program;
}

void addTexture(const std::string& _name, const std::string& _filename, bool _vFlip, TextureFilter _filter, TextureWrap _wrap) {
    Texture* tex = new Texture();
    if (tex->load(_filename, _vFlip, _filter, _wrap))
        main_scene->textures[_name] = tex;
    else
        delete tex;
}

void addTexture(const std::string& _name, const vera::Image& _image, TextureFilter _filter, TextureWrap _wrap) {
    Texture* tex = new Texture();
    if (tex->load(_image, _filter, _wrap))
        main_scene->textures[_name] = tex;
    else
        delete tex;
}

Texture* texture(const std::string _name) {
    vera::TexturesMap::iterator it = main_scene->textures.find(_name);
    if (it != main_scene->textures.end())
        return it->second;
    return nullptr;
}

Texture* texture(const std::string _name, const std::string _uniform_name) {
    vera::TexturesMap::iterator it = main_scene->textures.find(_name);
    if (it != main_scene->textures.end())
        return texture(it->second, _uniform_name);
    return nullptr;
}

Texture* texture(Texture& _texture, const std::string _uniform_name) { return texture(&_texture, _uniform_name); }
Texture* texture(Texture* _texture, const std::string _uniform_name) {
    if (shaderPtr == nullptr)
        shaderPtr = fillShader();
    
    std::string uniform_name = _uniform_name;
    if (_uniform_name.size() == 0)
        uniform_name = "u_tex" + toString(shaderPtr->textureIndex);
    
    shaderPtr->addDefine("USE_TEXTURE", uniform_name);
    shaderPtr->setUniformTexture(uniform_name, _texture, shaderPtr->textureIndex );
    shaderPtr->setUniform(uniform_name + "Resolution", (float)_texture->getWidth(), (float)_texture->getHeight());
    shaderPtr->textureIndex++;

    return _texture;
}

// void loadModel( const std::string& _filename ) {
//     // TODO:
//     // 
// }

void model(const Mesh& _mesh, Shader* _program) { 
    Vbo* vbo = new Vbo( _mesh );
    model(vbo, _program);
    delete vbo;
}

void model(Vbo& _vbo, Shader* _program) { model(&_vbo, _program); }
void model(Vbo* _vbo, Shader* _program) {
    if (_program == nullptr) {
        if (shaderPtr != nullptr) {
            _program = shaderPtr;
        }
        else {
            _program = fillShader();
        }
    }

    if (shaderChange ) //&& 
        // shaderPtr != fill_shader && 
        // shaderPtr != stroke_shader &&
        // shaderPtr != points_shader) 
    {
        VertexLayout* vl = _vbo->getVertexLayout();
        if (vl->haveAttrib("color"))
            _program->addDefine("MODEL_VERTEX_COLOR", "v_color");
        else
            _program->delDefine("MODEL_VERTEX_COLOR");

        if (vl->haveAttrib("normal"))
            _program->addDefine("MODEL_VERTEX_NORMAL", "v_normal");
        else
            _program->delDefine("MODEL_VERTEX_COLOR");

        if (vl->haveAttrib("texcoord"))
            _program->addDefine("MODEL_VERTEX_TEXCOORD", "v_texcoord");
        else
            _program->delDefine("MODEL_VERTEX_TEXCOORD");

        if (vl->haveAttrib("tangent"))
            _program->addDefine("MODEL_VERTEX_TANGENT", "v_tangent");
        else
            _program->delDefine("MODEL_VERTEX_TANGENT");

        if (_program->isDirty())
            _program->reload();

        shaderChange = false;
    }

    shader(_program);

    if (_vbo->getDrawMode() == GL_LINES || _vbo->getDrawMode() == GL_LINE_LOOP || _vbo->getDrawMode() == GL_LINE_STRIP)
        _program->setUniform("u_color", stroke_color);

    _vbo->render(_program);
}

// 3D Scene
void scene(Scene& _scene) { scene( &_scene ); }
void scene(Scene* _scene) { main_scene = _scene; }
Scene* scene() { return main_scene; }

void lights() { lights_enabled = true; }
void noLights() { lights_enabled = false; }

Light* createLight() {
    return new Light();
}

void addLight(const std::string& _name, Light& _light) { 
    main_scene->lights[_name] = &_light; 
}

void addLight(const std::string& _name, Light* _light) { 
    main_scene->lights[_name] = _light;
}

void addLabel(Label& _label) { addLabel(&_label); }
void addLabel(Label* _label) {
    if (main_scene->activeFont == nullptr)
        main_scene->activeFont = main_scene->getDefaultFont();
    main_scene->labels.push_back( _label );
}

// Ephymeral Label
void addLabel(const char* _text, glm::vec3 _position, LabelType _type, float _margin) {
    glm::vec3 pos = worldMatrix() * glm::vec4(_position, 1.0f);
    addLabel( new vera::Label(std::string(_text), pos, _type, _margin) );
}
void addLabel(const std::string& _text, glm::vec3 _position, LabelType _type, float _margin) {
    glm::vec3 pos = worldMatrix() * glm::vec4(_position, 1.0f);
    addLabel( new vera::Label(_text, pos, _type, _margin) );
}

//  Anchored Labels
void addLabel(const char* _text, glm::vec3* _position, LabelType _type, float _margin) {
    addLabel( new vera::Label(_text, _position, _type, _margin) );
}
void addLabel(const char* _text, Node* _node, LabelType _type, float _margin) {
    addLabel( new vera::Label(_text, _node, _type, _margin) );
}
void addLabel(const char* _text, Model* _model, LabelType _type, float _margin) {
    addLabel( new vera::Label(_text, _model, _type, _margin) );
}

void addLabel(const std::string& _text, glm::vec3* _position, LabelType _type, float _margin) {
    addLabel( new vera::Label(_text, _position, _type, _margin) );
}
void addLabel(const std::string& _text, Node* _node, LabelType _type, float _margin) {
    addLabel( new vera::Label(_text, _node, _type, _margin) );
}
void addLabel(const std::string& _text, Model* _model, LabelType _type, float _margin) {
    addLabel( new vera::Label(_text, _model, _type, _margin) );
}
void addLabel(std::function<std::string(void)> _func, glm::vec3* _position, LabelType _type, float _margin) {
    addLabel( new vera::Label(_func, _position, _type, _margin) );
}
void addLabel(std::function<std::string(void)> _func, Node* _node, LabelType _type, float _margin) {
    addLabel( new vera::Label(_func, _node, _type, _margin) );
}
void addLabel(std::function<std::string(void)> _func, Model* _model, LabelType _type, float _margin) {
    addLabel( new vera::Label(_func, _model, _type, _margin) );
}

LabelSettings& labelSettings() { return main_scene->labelSettings; }

void labelRadialCenter(float _x, float _y) { labelSettings().radialCenter = glm::vec2(_x, _y); }
void labelRadialCenter(const glm::vec2& _center) { labelSettings().radialCenter = _center; }
void labelRadialCenter(const glm::vec3& _center) { 
    Camera *cam = main_scene->activeCamera != nullptr ? main_scene->activeCamera : main_scene->lastCamera;
    glm::vec2 center = cam->worldToScreen(_center, worldMatrixPtr());
    labelRadialCenter( center );
}

void labels() {
    if (main_scene->activeFont == nullptr)
        main_scene->activeFont = main_scene->getDefaultFont();

    bool enabledCamera = main_scene->activeCamera != nullptr;

    Camera *cam = enabledCamera? main_scene->activeCamera : main_scene->lastCamera;

    Label::updateList( main_scene->labels, cam, main_scene->activeFont );

    if (enabledCamera)
        resetCamera();

    main_scene->activeFont->setEffect( EFFECT_NONE );
    main_scene->activeFont->setColor( fill_color );
    
    Label::renderList( main_scene->labels, main_scene->activeFont );

    if (enabledCamera)
        setCamera(cam);
}

void cleanLabels() {
    for (size_t i = 0; i < main_scene->labels.size(); i++)
        delete main_scene->labels[i];
    main_scene->labels.clear();
}

int labelAt(float _x, float _y) {
    for (size_t i = 0; i < main_scene->labels.size(); i++)
        if (main_scene->labels[i]->contains(_x, _y))
            return i;
    
    return -1;
}

Label* label(size_t _index) {
    if (_index >= main_scene->labels.size())
        return nullptr;

    return main_scene->labels[_index];
}

};
