#include "vera/ops/draw.h"

#include "vera/ops/meshes.h"
#include "vera/ops/fs.h"
#include "vera/ops/string.h"
#include "vera/xr/holoPlay.h"
#include "vera/window.h"

#include "vera/shaders/defaultShaders.h"

#include "glm/gtc/matrix_transform.hpp"

#include <stack>

namespace vera {

Shader*     shaderPtr       = nullptr;
bool        shaderChange    = true; 

HorizontalAlign shapeHAlign = ALIGN_CENTER;
VerticalAlign   shapeVAlign = ALIGN_MIDDLE;

glm::vec4   fill_color      = glm::vec4(1.0f);
Shader*     fill_shader     = nullptr;
bool        fill_enabled    = true;

float       points_size     = 10.0f;
int         points_shape    = 0.0;
Shader*     points_shader   = nullptr;

glm::vec4   stroke_color    = glm::vec4(1.0f);
bool        stroke_enabled  = true;

Shader*     billboard_shader = nullptr;
Vbo*        billboard_vbo    = new Vbo( rectMesh(0.0,0.0,1.0,1.0) );

Scene*      scene           = new Scene();

bool        lights_enabled  = false;

glm::mat4   matrix_world    = glm::mat4(1.0f);
std::stack<glm::mat4> matrix_stack;

void print(const std::string& _text) { std::cout << _text << std::endl; }
void frameRate(int _fps) { setFps(_fps); }

float pixelDensity() { return getPixelDensity(); }
void pixelDensity(float _density) { setPixelDensity(_density); }

bool fullscreen() { return isFullscreen(); }
void fullscreen(bool _fullscreen) { setFullscreen(_fullscreen); }

void resetMatrix() { matrix_world = glm::mat4(1.0f); }
void applyMatrix(const glm::mat3& _mat ) { matrix_world = _mat; }
void applyMatrix(const glm::mat4& _mat ) { matrix_world = glm::mat4(_mat); };

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
    CamerasMap::iterator it = scene->cameras.find(_name);
    if (it == scene->cameras.end()) {
        cam = new Camera();
        scene->cameras[_name] = cam;
    }
    else 
        cam = it->second;

    cam->setViewport(getWindowWidth(), getWindowHeight());
    glEnable(GL_DEPTH_TEST);
    return cam;
}

void setCamera(Camera& _camera) { setCamera(&_camera); }
void setCamera(Camera* _camera) {
    scene->activeCamera = _camera;
    glEnable(GL_DEPTH_TEST);
};

void resetCamera() {  
    scene->activeCamera = nullptr;
    glDisable(GL_DEPTH_TEST);
};

void addCamera(Camera& _camera, const std::string& _name) { addCamera(&_camera, _name); }
void addCamera(Camera* _camera, const std::string& _name) { scene->cameras[_name] = _camera; }
Camera* getCamera() { return scene->activeCamera; }

void perspective(float _fovy, float _aspect, float _near, float _far) {
    Camera* cam = createCamera("perspective");
    cam->setProjection( PERSPECTIVE );
    // cam->setProjection( glm::perspective(_fovy, _aspect, _near, _far) );
    cam->setAspect(_aspect);
    cam->setFOV(_fovy);
    cam->setClipping(_near, _far);
    scene->activeCamera = cam;
}

void ortho(float _left, float _right, float _bottom, float _top,  float _near, float _far) {
    Camera* cam = createCamera("ortho");
    cam->setProjection( glm::ortho(  _left , _right, _bottom, _top, _near, _top) );
    scene->activeCamera = cam;
}

glm::mat4 getProjectionViewWorldMatrix() {
    if (scene->activeCamera)
        return scene->activeCamera->getProjectionViewMatrix() * matrix_world; 
    else
        return getFlippedOrthoMatrix() * matrix_world;
}

const glm::mat4& getProjectionViewMatrix() {
    if (scene->activeCamera)
        return scene->activeCamera->getProjectionViewMatrix(); 
    else
        return getFlippedOrthoMatrix();
}

const glm::mat4& getProjectionMatrix() {
    if (scene->activeCamera)
        return scene->activeCamera->getProjectionMatrix(); 
    else
        return getFlippedOrthoMatrix();
}

const glm::mat4& getViewMatrix() {
    if (scene->activeCamera)
        return scene->activeCamera->getViewMatrix(); 
    else
        return getFlippedOrthoMatrix();
}

const glm::mat4& getWorldMatrix() { return matrix_world; }
glm::mat4* getWorldMatrixPtr() { return &matrix_world; }

Shader* getPointShader() {
    if (points_shader == nullptr) {
        points_shader = new Shader();
        points_shader->setSource( getDefaultSrc(FRAG_POINTS), getDefaultSrc(VERT_POINTS) );
    }

    return points_shader;
}

Shader* getFillShader() {
    if (fill_shader == nullptr) {
        fill_shader = new Shader();
        fill_shader->setSource( getDefaultSrc(FRAG_FILL), getDefaultSrc(VERT_FILL) );
    }
    
    return fill_shader;
}

void clear() { clear( glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) ); }
void clear( float _brightness ) { clear( glm::vec4(_brightness, _brightness, _brightness, 1.0f) ); }
void clear( const glm::vec3& _color ) { clear( glm::vec4(_color, 1.0f) ); }
void clear( const glm::vec4& _color ) {
    glClearColor(_color.r, _color.g, _color.b, _color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
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

    getFillShader();
    if (shaderPtr == nullptr || shaderPtr != fill_shader)
        shaderPtr = fill_shader;
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

    getFillShader();
    if (shaderPtr == nullptr || shaderPtr != fill_shader)
        shaderPtr = fill_shader;
}
void strokeWeight( float _weight) { glLineWidth(_weight); }

void pointSize( float _size ) { points_size = _size; }
void pointShape( PointShape _shape) { points_shape = _shape; }


// POINTS
//
void points(const std::vector<glm::vec2>& _positions, Shader* _program) {
    if (_program == nullptr)
        _program = getPointShader();
    
    shader(_program);

#if defined(__EMSCRIPTEN__)
    Vbo vbo = _positions;
    vbo.setDrawMode(POINTS);
    vbo.render(_program);
#else

    #if !defined(PLATFORM_RPI) && !defined(DRIVER_GBM) && !defined(_WIN32) && !defined(__EMSCRIPTEN__)
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
        _program = getPointShader();
    
    shader(_program);

#if defined(__EMSCRIPTEN__)
    Vbo vbo = _positions;
    vbo.setDrawMode(POINTS);
    vbo.render(_program);
#else
    #if !defined(PLATFORM_RPI) && !defined(DRIVER_GBM) && !defined(_WIN32)
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
    if (_program == nullptr)
        _program = getFillShader();
   
    shader(_program);
    _program->setUniform("u_color", stroke_color);

#if defined(__EMSCRIPTEN__)
    Vbo vbo = _positions;
    vbo.setDrawMode(LINE_STRIP);
    vbo.render(_program);
#else
    const GLint location = _program->getAttribLocation("a_position");
    if (location != -1) {
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, 2, GL_FLOAT, false, 0,  _positions.data());
        glDrawArrays(GL_LINE_STRIP, 0, _positions.size());
        glDisableVertexAttribArray(location);
    }
#endif
};

void line(const glm::vec3& _a, const glm::vec3& _b, Shader* _program) {
    std::vector<glm::vec3> pts = {_a, _b};
    line(pts, _program);
}

void line(const std::vector<glm::vec3>& _positions, Shader* _program) {
    if (_program == nullptr)
        _program = getFillShader();

    shader(_program);
    _program->setUniform("u_color", stroke_color);

#if defined(__EMSCRIPTEN__)
    Vbo vbo = _positions;
    vbo.setDrawMode(LINE_STRIP);
    vbo.render(_program);
#else
    const GLint location = _program->getAttribLocation("a_position");
    if (location != -1) {
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, 0, (const void*)_positions.data());
        glDrawArrays(GL_LINE_STRIP, 0, _positions.size());
        glDisableVertexAttribArray(location);
    }

#endif
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

// IMAGE
// 
Vbo* getBillboard() { return billboard_vbo; }

Image loadImage(const std::string& _name) {
    Image img;
    img.load(_name);
    return img;
}

void image(const Image &_img) { image(&_img); }
void image(const Image *_img) {
    std::string name = _img->getFilePath();
    TexturesMap::iterator it = scene->textures.find( name );
    if (it == scene->textures.end())
        scene->textures[ name ] = new Texture();
    scene->textures[ name ]->load( _img );
    image( scene->textures[ name ] );
}

void image(const Image &_img, float _x, float _y, float _width, float _height) { image(&_img, _x, _y, _width, _height); }
void image(const Image *_img, float _x, float _y, float _width, float _height) {
    std::string name = _img->getFilePath();
    TexturesMap::iterator it = scene->textures.find( name );
    if (it == scene->textures.end())
        scene->textures[ name ] = new Texture();
    scene->textures[ name ]->load( _img );
    image( scene->textures[ name ], _x, _y, _width, _height );
}

void image(const std::string &_path) {
    TexturesMap::iterator it = scene->textures.find( _path );
    if (it == scene->textures.end()) {
        Texture* tex = new Texture();
        tex->load( _path );
        scene->textures[ _path ] = tex;
    }
    image( scene->textures[ _path ] );
}

void image(const std::string &_path, float _x, float _y, float _width, float _height) {
    TexturesMap::iterator it = scene->textures.find( _path );
    if (it == scene->textures.end()) {
        Texture* tex = new Texture();
        tex->load( _path );
        scene->textures[ _path ] = tex;
    }
    image( scene->textures[ _path ], _x, _y, _width, _height );
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
Font* getDefaultFont() { return scene->getDefaultFont(); }
Font* loadFont(const std::string& _file, const std::string& _name) {
    scene->addFont(_name, _file);
    return scene->fonts[_name];
}
void  addFont(Font& _font, const std::string _name) { addFont(&_font, _name); }
void  addFont(Font* _font, const std::string _name) { scene->fonts[_name] = _font; }

Font* getFont() { 
    if (!scene->activeFont)
        scene->activeFont = scene->getDefaultFont();

    return scene->activeFont; 
}
Font* getFont(const std::string& _name) { 
    FontsMap::iterator it = scene->fonts.find(_name);
    if (it != scene->fonts.end())
        return it->second;
    return nullptr;
}

float getFontHeight() {
    return getFont()->getHeight();
}

//  FONT
//
Font* textFont(const std::string& _name) { 
    if (_name == "default")
        scene->getDefaultFont();

    FontsMap::iterator it = scene->fonts.find(_name);
    if (it != scene->fonts.end())
        scene->activeFont = it->second;

    return scene->activeFont; 
}

void textAlign(HorizontalAlign _align, Font* _font) {
    if (_font == nullptr)
        _font = getFont();
    _font->setAlign( _align );
}

void textAlign(VerticalAlign _align, Font* _font) {
    if (_font == nullptr)
        _font = getFont();
    _font->setAlign( _align );
}

void textAngle(float _angle, Font* _font) {
    if (_font == nullptr)
        _font = getFont();
    _font->setAngle( _angle );
}

void textSize(float _size, Font* _font) { 
    if (_font == nullptr)
        _font = getFont();

    if (getWindowStyle() == LENTICULAR)
        _font->setSize(_size * 3.0f );
    else
        _font->setSize(_size );
}

void text(const std::string& _text, const glm::vec2& _pos, Font* _font) { text(_text, _pos.x, _pos.y, _font); }
void text(const std::string& _text, float _x, float _y, Font* _font) {
    if (_font == nullptr)
        _font = getFont();
    _font->setColor( fill_color );
    _font->render(_text, _x, _y);
}

// SHAPES
// 
void triangles(const std::vector<glm::vec2>& _positions, Shader* _program) {
    if (_program == nullptr)
        _program = getFillShader();;

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
        _program = getFillShader();;

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

    std::vector<glm::vec2> tris = {     coorners[0], coorners[1], coorners[2],
                                        coorners[2], coorners[3], coorners[0] };

    if (_program == nullptr) {
        if (fill_enabled) triangles(tris);
        if (stroke_enabled) line(coorners);
    }
    else
        triangles(tris, _program);
}

// SHADER
//
Shader loadShader(const std::string& _fragFile, const std::string& _vertFile) {
    Shader s;
    s.setSource(loadGlslFrom(_fragFile), loadGlslFrom(_vertFile));
    return s;
}

Shader createShader(const std::string& _fragSrc, const std::string& _vertSrc) {
    Shader s;
    // s.addDefine("MODEL_VERTEX_COLOR", "v_color");
    // s.addDefine("MODEL_VERTEX_NORMAL", "v_normal");
    // s.addDefine("MODEL_VERTEX_TEXCOORD", "v_texcoord");
    // s.addDefine("MODEL_VERTEX_TANGENT", "v_tangent");

    if (!_fragSrc.empty() && _vertSrc.empty())
        s.setSource(_fragSrc, getDefaultSrc(VERT_DEFAULT_SCENE));
    else if (_fragSrc.empty())
        s.setSource(getDefaultSrc(FRAG_DEFAULT_SCENE), getDefaultSrc(VERT_DEFAULT_SCENE));
    else
        s.setSource(_fragSrc, _vertSrc);

    return s;
}

Shader createShader(DefaultShaders _frag, DefaultShaders _vert) {
    Shader s;
    // s.addDefine("MODEL_VERTEX_COLOR", "v_color");
    // s.addDefine("MODEL_VERTEX_NORMAL", "v_normal");
    // s.addDefine("MODEL_VERTEX_TEXCOORD", "v_texcoord");
    // s.addDefine("MODEL_VERTEX_TANGENT", "v_tangent");
    s.setSource(getDefaultSrc(_frag), getDefaultSrc(_vert));
    return s;
}

void addShader(const std::string& _name, Shader* _shader) { scene->shaders[_name] = _shader; }
void addShader(const std::string& _name, Shader& _shader) { addShader(_name, &_shader); }
void addShader(const std::string& _name, const std::string& _fragSrc, const std::string& _vertSrc) {
    scene->shaders[_name] = new Shader();
    if (!_fragSrc.empty() && _vertSrc.empty())
        scene->shaders[_name]->setSource(_fragSrc, getDefaultSrc(VERT_DEFAULT_SCENE));
    else if (_fragSrc.empty())
        scene->shaders[_name]->setSource(getDefaultSrc(FRAG_DEFAULT_SCENE), getDefaultSrc(VERT_DEFAULT_SCENE));
    else
        scene->shaders[_name]->setSource(_fragSrc, _vertSrc);
}

Shader* getShader(const std::string& _name) {
    ShadersMap::iterator it = scene->shaders.find(_name);
    if (it != scene->shaders.end())
        return it->second;
    return nullptr;
}

Shader* getShader() { return shaderPtr; }
void resetShader() { shaderPtr = nullptr; }

void shader(const std::string& _name) {
    Shader* shdr = getShader(_name);
    if (shdr)
        shader(shdr);
}

void shader(Shader& _program) { shader(&_program); }
void shader(Shader* _program) {
    if (shaderPtr != fill_shader || shaderPtr != points_shader) {
        shaderPtr = _program; 
        shaderChange = true;
    }

    _program->textureIndex = 0;
    _program->use();

    _program->setUniform("u_date", getDate() );
    _program->setUniform("u_resolution", (float)getWindowWidth(), (float)getWindowHeight() );
    _program->setUniform("u_mouse", (float)getMouseX(), (float)getMouseY() );
    _program->setUniform("u_time", (float)getTimeSec() );
    _program->setUniform("u_delta", (float)getDelta() );

    if (_program == fill_shader)
        _program->setUniform("u_color", fill_color);
    else if (_program == points_shader) {
        _program->setUniform("u_size", points_size);
        _program->setUniform("u_shape", points_shape);
        _program->setUniform("u_color", fill_color);
        #if !defined(PLATFORM_RPI) && !defined(DRIVER_GBM) && !defined(_WIN32) && !defined(__EMSCRIPTEN__)
        glEnable(GL_POINT_SPRITE);
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
        #endif
    }

    if (scene->activeCamera) {
        _program->setUniform("u_modelViewProjectionMatrix", scene->activeCamera->getProjectionViewMatrix() * matrix_world );
        _program->setUniform("u_projectionMatrix", scene->activeCamera->getProjectionMatrix());
        _program->setUniform("u_normalMatrix", scene->activeCamera->getNormalMatrix());
        _program->setUniform("u_viewMatrix", scene->activeCamera->getViewMatrix() );

        _program->setUniform("u_camera", -scene->activeCamera->getPosition() );
        _program->setUniform("u_cameraDistance", scene->activeCamera->getDistance());
        _program->setUniform("u_cameraNearClip", scene->activeCamera->getNearClip());
        _program->setUniform("u_cameraFarClip", scene->activeCamera->getFarClip());
        _program->setUniform("u_cameraEv100", scene->activeCamera->getEv100());
        _program->setUniform("u_cameraExposure", scene->activeCamera->getExposure());
        _program->setUniform("u_cameraAperture", scene->activeCamera->getAperture());
        _program->setUniform("u_cameraShutterSpeed", scene->activeCamera->getShutterSpeed());
        _program->setUniform("u_cameraSensitivity", scene->activeCamera->getSensitivity());
        _program->setUniform("u_cameraChange", scene->activeCamera->bChange);
        _program->setUniform("u_iblLuminance", 30000.0f * scene->activeCamera->getExposure());
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
        if (scene->lights.size() == 1) {
            LightsMap::iterator it = scene->lights.begin();

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
            for (LightsMap::iterator it = scene->lights.begin(); it != scene->lights.end(); ++it) {
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
}

void addTexture(const std::string& _name, const std::string& _filename, bool _vFlip, TextureFilter _filter, TextureWrap _wrap) {
    Texture* tex = new Texture();
    if (tex->load(_filename, _vFlip, _filter, _wrap))
        scene->textures[_name] = tex;
    else
        delete tex;
}

void addTexture(const std::string& _name, const vera::Image& _image, TextureFilter _filter, TextureWrap _wrap) {
    Texture* tex = new Texture();
    if (tex->load(_image, _filter, _wrap))
        scene->textures[_name] = tex;
    else
        delete tex;
}

Texture* getTexture(const std::string& _name) {
    vera::TexturesMap::iterator it = scene->textures.find(_name);
    if (it != scene->textures.end())
        return it->second;
    else
        return nullptr;
}

void texture(const std::string _name, const std::string _uniform_name) {
    vera::TexturesMap::iterator it = scene->textures.find(_name);
    if (it != scene->textures.end())
        texture(it->second, _uniform_name);
}
void texture(Texture& _texture, const std::string _uniform_name) { texture(&_texture, _uniform_name); }
void texture(Texture* _texture, const std::string _uniform_name) {
    if (shaderPtr == nullptr)
        shaderPtr = getFillShader();
    
    std::string uniform_name = _uniform_name;
    if (_uniform_name.size() == 0)
        uniform_name = "u_tex" + toString(shaderPtr->textureIndex);
    shaderPtr->addDefine("USE_TEXTURE", uniform_name);
    shaderPtr->setUniformTexture(uniform_name, _texture, shaderPtr->textureIndex );
    shaderPtr->setUniform(uniform_name + "Resolution", (float)_texture->getWidth(), (float)_texture->getHeight());
    shaderPtr->textureIndex++;
}

void loadModel( const std::string& _filename ) {
    // TODO:
    // 
}

void model(Vbo& _vbo, Shader* _program) { model(&_vbo, _program); }
void model(Vbo* _vbo, Shader* _program) {
    if (_program == nullptr) {
        if (shaderPtr != nullptr)
            _program = shaderPtr;
        else
            _program = getFillShader();
    }

    if (shaderChange && 
        shaderPtr != fill_shader && 
        shaderPtr != points_shader) {
        VertexLayout* vl = _vbo->getVertexLayout();
        if (vl->haveAttrib("color"))
            _program->addDefine("MODEL_VERTEX_COLOR", "v_color");
        // else
            // _program->delDefine("MODEL_VERTEX_COLOR");

        if (vl->haveAttrib("normal"))
            _program->addDefine("MODEL_VERTEX_NORMAL", "v_normal");
        // else
            // _program->delDefine("MODEL_VERTEX_COLOR");

        if (vl->haveAttrib("texcoord"))
            _program->addDefine("MODEL_VERTEX_TEXCOORD", "v_texcoord");
        // else
            // _program->delDefine("MODEL_VERTEX_TEXCOORD");

        if (vl->haveAttrib("tangent"))
            _program->addDefine("MODEL_VERTEX_TANGENT", "v_tangent");
        // else
            // _program->delDefine("MODEL_VERTEX_TEXCOORD");

        shaderChange = false;
    }

    shader(_program);

    if (_vbo->getDrawMode() == GL_LINES || _vbo->getDrawMode() == GL_LINE_LOOP || _vbo->getDrawMode() == GL_LINE_STRIP)
        _program->setUniform("u_color", stroke_color);

    _vbo->render(_program);
}

// 3D Scene
void setScene(Scene& _scene) { setScene( &_scene ); }
void setScene(Scene* _scene) { scene = _scene; }
Scene* getScene() { return scene; }

void lights() { lights_enabled = true; }
void noLights() { lights_enabled = false; }

Light* createLight(const std::string& _name) {
    Light* light = new Light();
    addLight(light, _name);
    return light;
}

void addLight(Light& _light, const std::string& _name) { 
    scene->lights[_name] = &_light; 
}

void addLight(Light* _light, const std::string& _name) { 
    scene->lights[_name] = _light;
}

void addLabel(Label& _label) { addLabel(&_label); }
void addLabel(Label* _label) {
    if (scene->activeFont == nullptr)
        scene->activeFont = getDefaultFont();
    scene->labels.push_back( _label );
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

void labels() {
    if (scene->activeFont == nullptr)
        scene->activeFont = getDefaultFont();

    Camera *cam = getCamera();

    for (size_t i = 0; i < scene->labels.size(); i++)
        scene->labels[i]->update( cam, scene->activeFont );

    resetCamera();

    scene->activeFont->setEffect( EFFECT_NONE );
    scene->activeFont->setColor( fill_color );
    for (size_t i = 0; i < scene->labels.size(); i++)
        scene->labels[i]->render( scene->activeFont );

    setCamera(cam);
}

void cleanLabels() {
    for (size_t i = 0; i < scene->labels.size(); i++)
        delete scene->labels[i];
    scene->labels.clear();
}

int labelAt(float _x, float _y) {
    for (size_t i = 0; i < scene->labels.size(); i++)
        if (scene->labels[i]->contains(_x, _y))
            return i;
    
    return -1;
}

Label* label(size_t _index) {
    if (_index >= scene->labels.size())
        return nullptr;

    return scene->labels[_index];
}

};
