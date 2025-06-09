#pragma once

#include <vector>

#include "vera/gl/vbo.h"
#include "vera/gl/shader.h"
#include "vera/shaders/defaultShaders.h"

#include "vera/types/boundingBox.h"
#include "vera/types/image.h"
#include "vera/types/font.h"
#include "vera/types/line.h"
#include "vera/types/mesh.h"
#include "vera/types/scene.h"
#include "vera/types/polyline.h"
#include "vera/types/triangle.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include <emscripten/html5.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

namespace vera {

#ifndef CIRCLE_RESOLUTION
#define CIRCLE_RESOLUTION 36
#endif

enum PointShape {
    SQUARE_SHAPE = 0,
    SQUARE_OUTLINE_SHAPE,
    DOT_SHAPE,
    DOT_OUTLINE_SHAPE,
    CROSS_SHAPE,
    X_SHAPE
};

// GENERAL GL STATE
// ---------------------------------
bool fullscreen();
void fullscreen(bool _fullscreen);

void print(const std::string& _text);
void frameRate(int _fps);

// cursor()
// noCursor()

void flagChange();
bool haveChanged();
void resetChange();

float displayDensity();
float pixelDensity();
void  pixelDensity(float _density);

void clear( float _brightness );
void clear( const glm::vec3& _color );
void clear( const glm::vec4& _color );

void background();
void background( int _brightness );
void background( int _red, int _green, int _blue);
void background( int _red, int _green, int _blue, int _alpha);
void background( float _brightness );
void background( float _red, float _green, float _blue);
void background( float _red, float _green, float _blue, float _alpha);
void background( const glm::vec3& _color );
void background( const glm::vec4& _color );

bool getBackgroundEnabled();
glm::vec4 getBackground();

void fill( float _brightness );
void fill( float _red, float _green, float _blue);
void fill( float _red, float _green, float _blue, float _alpha);
void fill( const glm::vec3& _color );
void fill( const glm::vec4& _color );
void noFill();
const glm::vec4& getFillColor();

void stroke( float _brightness );
void stroke( float _red, float _green, float _blue);
void stroke( float _red, float _green, float _blue, float _alpha);
void stroke( const glm::vec3& _color );
void stroke( const glm::vec4& _color );
void noStroke();
void strokeWeight(float _weight);
const glm::vec4& getStrokeColor();

// MATRICES
// -----------------------------
void resetMatrix();
void applyMatrix(const glm::mat3& _mat );
void applyMatrix(const glm::mat4& _mat );
glm::mat4 getProjectionViewWorldMatrix();
const glm::mat4& getProjectionViewMatrix();
const glm::mat4& getProjectionMatrix();
const glm::mat4& getViewMatrix();
const glm::mat4& getWorldMatrix();
glm::mat4* getWorldMatrixPtr();

void rotate(float _rad);
void rotateX(float _rad);
void rotateY(float _rad);
void rotateZ(float _rad);

void scale(float _s);
void scale(float _x, float _y, float _z = 1.0f);
void scale(const glm::vec2& _s);
void scale(const glm::vec3& _s);

void translate(float _x, float _y, float _z = 0.0f);
void translate(const glm::vec2& _t);
void translate(const glm::vec3& _t);

void push();
void pop();

// colorMode()
// erase()
// noErase()

// POINTS
// -----------------------------
void pointSize( float _size );
void pointShape( PointShape _shape );
void points(const std::vector<glm::vec2>& _positions, Shader* _program = nullptr);
void points(const std::vector<glm::vec3>& _positions, Shader* _program = nullptr);
void points(const Line& _line, Shader* _program = nullptr);
void points(const Triangle& _triangle, Shader* _program = nullptr);
void points(const BoundingBox& _bbox, Shader* _program = nullptr);
void pointsBoundingBox(const glm::vec4& _bbox, Shader* _program = nullptr);

// Linex
void line(float _x1, float _y1, float _x2, float _y2, Shader* _program = nullptr);
void line(const glm::vec2& _a, const glm::vec2& _b, Shader* _program = nullptr);
void line(const std::vector<glm::vec2>& _positions, Shader* _program = nullptr);
void line(float _x1, float _y1, float _z1, float _x2, float _y2, float _z2, Shader* _program = nullptr);
void line(const glm::vec3& _a, const glm::vec3& _b, Shader* _program = nullptr);
void line(const std::vector<glm::vec3>& _positions, Shader* _program = nullptr);
void line(const Line& _line, Shader* _program = nullptr);
void line(const Polyline& _polyline, Shader* _program = nullptr);
void line(const Triangle& _triangle, Shader* _program = nullptr);
void line(const BoundingBox& _bbox, Shader* _program = nullptr);
void lineBoundingBox(const glm::vec4& _bbox, Shader* _program = nullptr);

// arc()

// 2D Primitives
void triangle(const glm::vec2& _center, float angle = 0.0, float _radius = 1.0,  Shader* _program = nullptr);
void triangle(const glm::vec3& _center, float angle = 0.0, float _radius = 1.0,  Shader* _program = nullptr);
void triangle(const glm::vec3& _center, glm::vec3 _up, float _radius = 1.0f, Shader* _program = nullptr);
void triangle(float _x1, float _y1, float _x2, float _y2, float _x3, float _y3, Shader* _program = nullptr);
void triangle(const glm::vec2& _a, const glm::vec2& _b, const glm::vec2& _c, Shader* _program = nullptr);
void triangle(float _x1, float _y1, float _z1, float _x2, float _y2, float _z2, float _x3, float _y3, float _z3, Shader* _program = nullptr);
void triangle(const glm::vec3& _a, const glm::vec3& _b, const glm::vec3& _c, Shader* _program = nullptr);

void triangles(const std::vector<glm::vec2>& _positions, Shader* _program = nullptr);
void triangles(const std::vector<glm::vec3>& _positions, Shader* _program = nullptr);

void rectAlign(HorizontalAlign _align);
void rectAlign(VerticalAlign _align);
void rect(float _x, float _y, float _w, float _h, Shader* _program = nullptr);
void rect(const glm::vec2& _pos, const glm::vec2& _size, Shader* _program = nullptr);
void rect(const BoundingBox& _bbox, Shader* _program = nullptr);

// quad()
// square()s
void circleResolution(int _res = CIRCLE_RESOLUTION);
void circle(float _x, float _y, float _r, Shader* _program = nullptr);
void circle(const glm::vec2& _pos, float _r, Shader* _program = nullptr);
// ellipse()

HorizontalAlign getRectHorizontalAlign();
VerticalAlign   getRectVerticalAlign();

// 3D Primitives
void plane(Shader* _program = nullptr);
void plane(float _size, Shader* _program = nullptr);
void plane(float _width, float _height, Shader* _program = nullptr);
void plane(float _width, float _height, int _res, Shader* _program = nullptr);
void plane(float _width, float _height, int _resX, int _resY, Shader* _program = nullptr);

void box(Shader* _program = nullptr);
void box(float _size, Shader* _program = nullptr);
void box(float _width, float _height, Shader* _program = nullptr);
void box(float _width, float _height, float _depth, Shader* _program = nullptr);

void sphere(Shader* _program = nullptr);
void sphere(float _radius, Shader* _program = nullptr);
void sphere(float _radius, int _res, Shader* _program = nullptr);

// cylinder()
// cone()
// ellipsoid()
// torus()
// p5.Geometry

// IMAGES
// -----------------------------
Image loadImage(const std::string& _name);
void image(const std::string &_path);
void image(const std::string &_path, float _x, float _y, float _width = 0, float _height = 0);
void image(const Image &_img);
void image(const Image *_img);
void image(const Image &_img, float _x, float _y, float _width = 0, float _height = 0);
void image(const Image *_img, float _x, float _y, float _width = 0, float _height = 0);
void image(const Texture &_tex);
void image(const Texture *_tex);
void image(const Texture &_tex, float _x, float _y, float _width = 0, float _height = 0);
void image(const Texture *_tex, float _x, float _y, float _width = 0, float _height = 0);
void image(const TextureStream &_stream);
void image(const TextureStream *_stream);
void image(const TextureStream &_stream, float _x, float _y, float _width = 0, float _height = 0, bool _debug = false);
void image(const TextureStream *_stream, float _x, float _y, float _width = 0, float _height = 0, bool _debug = false);
void image(const Fbo &_fbo);
void image(const Fbo *_fbo);
void image(const Fbo &_fbo, float _x, float _y, float _width = 0, float _height = 0);
void image(const Fbo *_fbo, float _x, float _y, float _width = 0, float _height = 0);
void imageDepth(const Fbo &_fbo, float _x, float _y, float _width = 0, float _height = 0, float _far = 100.0f, float _near = 0.001f);
void imageDepth(const Fbo *_fbo, float _x, float _y, float _width = 0, float _height = 0, float _far = 100.0f, float _near = 0.001f);
Vbo* getBillboard();

// tint(v1, v2, v3, [alpha])
// tint(value)
// tint(gray, [alpha])
// tint(values)
// tint(color)
// noTint()
// imageMode(mode)

// PIXELS
// -----------------------------
// pixels
// 
// blend(srcImage, sx, sy, sw, sh, dx, dy, dw, dh, blendMode)
// blend(sx, sy, sw, sh, dx, dy, dw, dh, blendMode)
// 
// copy(srcImage, sx, sy, sw, sh, dx, dy, dw, dh)
// copy(sx, sy, sw, sh, dx, dy, dw, dh)
//
// filter(filterType, [filterParam])
// filterType Constant: either THRESHOLD, GRAY, OPAQUE, INVERT, POSTERIZE, BLUR, ERODE, DILATE or BLUR. See Filters.js for docs on each available filter
// filterParam Number: an optional parameter unique to each filter, see above (Optional)
//
// loadPixels()
// get(int _x = 0, int _y = 0, int _width = 0, int _height = 0)
// set(int _x = 0, int _y = 0, const glm::vec4& _color)
// updatePixels(int _x = 0, int _y = 0, int _width = 0, int _height)

// FONT
// -----------------------------
Font* getFont();
Font* getFont(const std::string& _name);

Font* loadFont(const std::string& _file, const std::string& _name = "default");
void  addFont(Font& _font, const std::string _name);
void  addFont(Font* _font, const std::string _name);

// TEXT
// -----------------------------
// textLeading()
// textStyle()
// textAscent()
// textDescent()
// textWrap()

Font* textFont(const std::string& _name);
void textAlign(HorizontalAlign _align, Font* _font = nullptr);
void textAlign(VerticalAlign _align, Font* _font = nullptr);
void textAngle(float _angle, Font* _font = nullptr);
void textSize(float _size, Font* _font = nullptr);
float textWidth(const std::string& _text, Font* _font = nullptr);
float textHeight(Font* _font = nullptr);
float textHeight(const std::string& _text, Font* _font = nullptr);

BoundingBox textBoundingBox(const std::string& _text, float _x, float _y, Font* _font = nullptr);
BoundingBox textBoundingBox(const std::string& _text, const glm::vec2& _pos, Font* _font = nullptr);
BoundingBox textBoundingBox(const std::string& _text, const glm::vec3& _pos, Font* _font = nullptr);
void text(const std::string& _text, const glm::vec2& _pos, float _angle = 0.0f, Font* _font = nullptr);
void text(const std::string& _text, const glm::vec3& _pos, Font* _font = nullptr);
void text(const std::string& _text, const glm::vec2& _pos, Font* _font = nullptr );
void text(const std::string& _text, float _x, float _y, Font* _font = nullptr);
void textHighlight(const std::string& _text, const glm::vec2& _pos, const glm::vec4& _bg = glm::vec4(0.0, 0.0, 0.0, 1.0), Font* _font = nullptr);
void textHighlight(const std::string& _text, float _x, float _y, const glm::vec4& _bg = glm::vec4(0.0, 0.0, 0.0, 1.0), Font* _font = nullptr);


// SHADERS
// -----------------------------
Shader  loadShader(const std::string& _fragFile, const std::string& _vertFile);
Shader  createShader(const std::string& _fragSrc = "", const std::string& _vertSrc = "");
Shader  createShader(DefaultShaders _frag, DefaultShaders _vert);

void    addShader(const std::string& _name, Shader& _shader);
void    addShader(const std::string& _name, Shader* _shader);
void    addShader(const std::string& _name, const std::string& _fragSrc = "", const std::string& _vertSrc = "");

Shader* getShader();
Shader* getShader(const std::string& _name);
std::vector<std::string> getShaderNames();
Shader* getPointShader();
Shader* getStrokeShader();
Shader* getSpline2DShader();
Shader* getSpline3DShader();
Shader* getFillShader();
void    resetShader();
void    shader(Shader& _shader);
void    shader(Shader* _shader);
void    shader(const std::string& _name);

// TEXTURES
// -----------------------------
void    addTexture(const std::string& _name, const std::string& _filename, bool _vFlip = false, TextureFilter _filter = LINEAR, TextureWrap _wrap = REPEAT);
void    addTexture(const std::string& _name, const vera::Image& _image, TextureFilter _filter = LINEAR, TextureWrap _wrap = REPEAT);
Texture* getTexture(const std::string& _name);
void    texture(Texture& _texture, const std::string _uniform_name = "");
void    texture(Texture* _texture, const std::string _uniform_name = "");
void    texture(const std::string _name, const std::string _uniform_name = "");
// textureMode()
// textureWrap()

// Interaction
// -----------------------------
// debugMode()
// noDebugMode()

// SCENE
// -----------------------------
void setScene(Scene& scene);
void setScene(Scene* scene);
Scene* getScene();

// CAMERA
// -----------------------------
// camera()
void perspective(float _fovy, float _aspect, float _near, float _far);
void ortho(float _left, float _right, float _bottom, float _top,  float _near, float _far);
// frustum()
Camera* createCamera(const std::string& _name = "unnamed");
void addCamera(Camera& _camera, const std::string& _name = "unnamed");
void addCamera(Camera* _camera, const std::string& _name = "unnamed");
void setCamera(const std::string& _name);
void setCamera(Camera& _camera);
void setCamera(Camera* _camera);
void resetCamera();

Camera* getCamera();
Camera* getLastCamera();
Camera* getCamera(const std::string& _name);

// LIGHT
// -----------------------------
// ambientLight()
// specularColor()
// directionalLight()
// pointLight()
// lightFalloff()
// spotLight()
void lights();
void noLights();
Light* createLight(const std::string& _name = "default");
void addLight(Light& _light, const std::string& _name = "default");
void addLight(Light* _light, const std::string& _name = "default");

// MATERIAL
// -----------------------------
// normalMaterial()
// ambientMaterial()
// emissiveMaterial()
// specularMaterial()
// shininess()

// MODEL
// -----------------------------
// void loadModel( const std::string& _filename );
void model(Vbo& _vbo, Shader* _program = nullptr);
void model(Vbo* _vbo, Shader* _program = nullptr);
void model(const Mesh& _mesh, Shader* _program = nullptr);

// LABELS
// -----------------------------
void addLabel(Label& _label);
void addLabel(Label* _label);

// Ephemeral labels
void addLabel(const char* _text, glm::vec3 _position, LabelType _type = LABEL_CENTER, float _margin = 0.0f);
void addLabel(const std::string& _text, glm::vec3 _position, LabelType _type = LABEL_CENTER, float _margin = 0.0f);

// Anchor labels
void addLabel(const char* _text, glm::vec3* _position, LabelType _type = LABEL_CENTER, float _margin = 0.0f);
void addLabel(const char* _text, Node* _node, LabelType _type = LABEL_CENTER, float _margin = 0.0f);
void addLabel(const char* _text, Model* _node, LabelType _type = LABEL_CENTER, float _margin = 0.0f);
void addLabel(const std::string& _text, glm::vec3* _position, LabelType _type = LABEL_CENTER, float _margin = 0.0f);
void addLabel(const std::string& _text, Node* _node, LabelType _type = LABEL_CENTER, float _margin = 0.0f);
void addLabel(const std::string& _text, Model* _node, LabelType _type = LABEL_CENTER, float _margin = 0.0f);
void addLabel(std::function<std::string(void)> _func, glm::vec3* _position, LabelType _type = LABEL_CENTER, float _margin = 0.0f);
void addLabel(std::function<std::string(void)> _func, Node* _node, LabelType _type = LABEL_CENTER, float _margin = 0.0f);
void addLabel(std::function<std::string(void)> _func, Model* _model, LabelType _type = LABEL_CENTER, float _margin = 0.0f);

void labels();
void cleanLabels();

LabelSettings& labelSettings();
void labelRadialCenter(float _x, float _y);
void labelRadialCenter(const glm::vec2& _center);
void labelRadialCenter(const glm::vec3& _center);

int labelAt(float _x, float _y);
Label* label(size_t _index);

};
