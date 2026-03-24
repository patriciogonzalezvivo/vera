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
#ifdef SUPPORT_GSPLAT
#include "vera/types/gsplat.h"
#endif
#include "vera/types/scene.h"
#include "vera/types/polyline.h"
#include "vera/types/triangle.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include <emscripten/html5.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

// Drawing interface inspired by Processing (https://processing.org/) and p5.js (https://p5js.org/)
// Provides high-level 2D and 3D drawing primitives with state management.

namespace vera {

#ifndef CIRCLE_RESOLUTION
#define CIRCLE_RESOLUTION 36
#endif

/// Point rendering shapes
enum PointShape {
    SQUARE_SHAPE = 0,       ///< Filled square
    SQUARE_OUTLINE_SHAPE,   ///< Square outline
    DOT_SHAPE,              ///< Filled circle
    DOT_OUTLINE_SHAPE,      ///< Circle outline
    CROSS_SHAPE,            ///< Plus (+) shape
    X_SHAPE                 ///< X mark
};

/// Arc drawing modes
enum ArcMode {
    OPEN_MODE = 0,   ///< Arc outline only (no closing line)
    CHORD_MODE,      ///< Arc closed with straight line between endpoints
    PIE_MODE         ///< Arc closed with lines to center (pie slice)
};

// =============================================================================
// GENERAL GL STATE AND WINDOW CONTROL
// =============================================================================

/// Check if window is in fullscreen mode
/// @return True if fullscreen
bool fullscreen();

/// Set fullscreen mode
/// @param _fullscreen True to enable fullscreen
void fullscreen(bool _fullscreen);

/// Print text to console or debug output
/// @param _text Text to print
void print(const std::string& _text);

/// Set target frame rate (note: actual behavior depends on system)
/// @param _fps Target frames per second
void frameRate(int _fps);

/// Mark that content has changed and needs redraw
void flagChange();

/// Check if content has changed since last check
/// @return True if content was flagged as changed
bool haveChanged();

/// Reset the changed flag
void resetChange();

/// Get display density (typically 1.0 or 2.0 for high-DPI)
/// @return Display pixel density multiplier
float displayDensity();

/// Get current pixel density setting
/// @return Pixel density multiplier
float pixelDensity();

/// Set pixel density for high-DPI rendering
/// @param _density Density multiplier (e.g., 2.0 for Retina)
void  pixelDensity(float _density);

// =============================================================================
// BACKGROUND AND CLEARING
// =============================================================================

/// Clear screen to black
/// @param _brightness Grayscale brightness (0.0-1.0)
void clear( float _brightness );

/// Clear screen to color
/// @param _color RGB color
void clear( const glm::vec3& _color );

/// Clear screen to color with alpha
/// @param _color RGBA color
void clear( const glm::vec4& _color );

/// Clear and set background (automatically drawn each frame)
void background();

/// Set background to grayscale (0-255)
/// @param _brightness Grayscale value (0-255)
void background( int _brightness );

/// Set background to RGB (0-255)
/// @param _red Red component (0-255)
/// @param _green Green component (0-255)
/// @param _blue Blue component (0-255)
void background( int _red, int _green, int _blue);

/// Set background to RGBA (0-255)
/// @param _red Red component (0-255)
/// @param _green Green component (0-255)
/// @param _blue Blue component (0-255)
/// @param _alpha Alpha component (0-255)
void background( int _red, int _green, int _blue, int _alpha);

/// Set background to grayscale (0.0-1.0)
/// @param _brightness Grayscale value (0.0-1.0)
void background( float _brightness );

/// Set background to RGB (0.0-1.0)
/// @param _red Red component (0.0-1.0)
/// @param _green Green component (0.0-1.0)
/// @param _blue Blue component (0.0-1.0)
void background( float _red, float _green, float _blue);

/// Set background to RGBA (0.0-1.0)
/// @param _red Red component (0.0-1.0)
/// @param _green Green component (0.0-1.0)
/// @param _blue Blue component (0.0-1.0)
/// @param _alpha Alpha component (0.0-1.0)
void background( float _red, float _green, float _blue, float _alpha);

/// Set background to RGB
/// @param _color RGB color vector
void background( const glm::vec3& _color );

/// Set background to RGBA
/// @param _color RGBA color vector
void background( const glm::vec4& _color );

/// Check if automatic background clearing is enabled
/// @return True if background is enabled
bool getBackgroundEnabled();

/// Get current background color
/// @return Background color (RGBA)
glm::vec4 getBackground();

// =============================================================================
// FILL AND STROKE COLORS
// =============================================================================

/// Set fill color to grayscale (0.0-1.0)
/// @param _brightness Grayscale value (0.0-1.0)
void fill( float _brightness );

/// Set fill color to RGB (0.0-1.0)
/// @param _red Red component (0.0-1.0)
/// @param _green Green component (0.0-1.0)
/// @param _blue Blue component (0.0-1.0)
void fill( float _red, float _green, float _blue);

/// Set fill color to RGBA (0.0-1.0)
/// @param _red Red component (0.0-1.0)
/// @param _green Green component (0.0-1.0)
/// @param _blue Blue component (0.0-1.0)
/// @param _alpha Alpha component (0.0-1.0)
void fill( float _red, float _green, float _blue, float _alpha);

/// Set fill color to RGB
/// @param _color RGB color vector
void fill( const glm::vec3& _color );

/// Set fill color to RGBA
/// @param _color RGBA color vector
void fill( const glm::vec4& _color );

/// Disable filling for subsequent shapes
void noFill();

/// Get current fill color
/// @return Fill color (RGBA)
const glm::vec4& getFillColor();

/// Set stroke color to grayscale (0.0-1.0)
/// @param _brightness Grayscale value (0.0-1.0)
void stroke( float _brightness );

/// Set stroke color to RGB (0.0-1.0)
/// @param _red Red component (0.0-1.0)
/// @param _green Green component (0.0-1.0)
/// @param _blue Blue component (0.0-1.0)
void stroke( float _red, float _green, float _blue);

/// Set stroke color to RGBA (0.0-1.0)
/// @param _red Red component (0.0-1.0)
/// @param _green Green component (0.0-1.0)
/// @param _blue Blue component (0.0-1.0)
/// @param _alpha Alpha component (0.0-1.0)
void stroke( float _red, float _green, float _blue, float _alpha);

/// Set stroke color to RGB
/// @param _color RGB color vector
void stroke( const glm::vec3& _color );

/// Set stroke color to RGBA
/// @param _color RGBA color vector
void stroke( const glm::vec4& _color );

/// Disable stroking for subsequent shapes
void noStroke();

/// Get current stroke weight
/// @return Stroke weight in pixels
float strokeWeight();

/// Set stroke weight for subsequent shapes
/// @param _weight Stroke weight in pixels
void strokeWeight(float _weight);

/// Get current stroke color
/// @return Stroke color (RGBA)
const glm::vec4& getStrokeColor();

// =============================================================================
// TRANSFORMATION MATRICES
// =============================================================================

/// Reset world transformation matrix to identity
void resetMatrix();

/// Apply a 3x3 transformation matrix to current world matrix
/// @param _mat 3x3 transformation matrix
void applyMatrix(const glm::mat3& _mat );

/// Apply a 4x4 transformation matrix to current world matrix
/// @param _mat 4x4 transformation matrix
void applyMatrix(const glm::mat4& _mat );

/// Get combined projection-view-world matrix
/// @return Combined transformation matrix
glm::mat4 projectionViewWorldMatrix();

/// Get combined projection-view matrix
/// @return Combined projection-view matrix
const glm::mat4& projectionViewMatrix();

/// Get current projection matrix
/// @return Projection matrix
const glm::mat4& projectionMatrix();

/// Get current view matrix
/// @return View matrix
const glm::mat4& viewMatrix();

/// Get current world transformation matrix
/// @return World matrix
const glm::mat4& worldMatrix();

/// Get pointer to current world matrix (for direct manipulation)
/// @return Pointer to world matrix
glm::mat4* worldMatrixPtr();

/// Apply 2D rotation around Z-axis
/// @param _rad Rotation angle in radians
void rotate(float _rad);

/// Apply rotation around X-axis
/// @param _rad Rotation angle in radians
void rotateX(float _rad);

/// Apply rotation around Y-axis
/// @param _rad Rotation angle in radians
void rotateY(float _rad);

/// Apply rotation around Z-axis
/// @param _rad Rotation angle in radians
void rotateZ(float _rad);

/// Apply uniform scaling
/// @param _s Scale factor
void scale(float _s);

/// Apply non-uniform scaling (2D or 3D)
/// @param _x X-axis scale factor
/// @param _y Y-axis scale factor
/// @param _z Z-axis scale factor (default 1.0 for 2D operations)
void scale(float _x, float _y, float _z = 1.0f);

/// Apply 2D scaling from vector
/// @param _s Scale factors (x, y)
void scale(const glm::vec2& _s);

/// Apply 3D scaling from vector
/// @param _s Scale factors (x, y, z)
void scale(const glm::vec3& _s);

/// Apply translation (2D or 3D)
/// @param _x X-axis translation
/// @param _y Y-axis translation
/// @param _z Z-axis translation (default 0.0 for 2D operations)
void translate(float _x, float _y, float _z = 0.0f);

/// Apply 2D translation from vector
/// @param _t Translation vector (x, y)
void translate(const glm::vec2& _t);

/// Apply 3D translation from vector
/// @param _t Translation vector (x, y, z)
void translate(const glm::vec3& _t);

/// Push current transformation matrix onto stack
void push();

/// Pop transformation matrix from stack
void pop();

// colorMode()
// erase()
// noErase()

// =============================================================================
// POINT RENDERING
// =============================================================================

/// Set point rendering size
/// @param _size Size in pixels
void pointSize( float _size );

/// Set point rendering shape
/// @param _shape Shape from PointShape enum
void pointShape( PointShape _shape );

/// Draw multiple 2D points
/// @param _positions Array of 2D point positions
/// @param _program Optional custom shader (null = use default)
void points(const std::vector<glm::vec2>& _positions, Shader* _program = nullptr);

/// Draw multiple 3D points
/// @param _positions Array of 3D point positions
/// @param _program Optional custom shader (null = use default)
void points(const std::vector<glm::vec3>& _positions, Shader* _program = nullptr);

/// Draw points at endpoints of a line
/// @param _line Line to extract points from
/// @param _program Optional custom shader (null = use default)
void points(const Line& _line, Shader* _program = nullptr);

/// Draw points at vertices of a triangle
/// @param _triangle Triangle to extract points from
/// @param _program Optional custom shader (null = use default)
void points(const Triangle& _triangle, Shader* _program = nullptr);

/// Draw points at corners of a bounding box
/// @param _bbox Bounding box to extract points from
/// @param _program Optional custom shader (null = use default)
void points(const BoundingBox& _bbox, Shader* _program = nullptr);

/// Draw points at corners of a 2D bounding box (vec4: x, y, w, h)
/// @param _bbox 2D bounding box (x, y, width, height)
/// @param _program Optional custom shader (null = use default)
void pointsBoundingBox(const glm::vec4& _bbox, Shader* _program = nullptr);

// =============================================================================
// LINE RENDERING
// =============================================================================

/// Draw 2D line between two points
/// @param _x1 Start X coordinate
/// @param _y1 Start Y coordinate
/// @param _x2 End X coordinate
/// @param _y2 End Y coordinate
/// @param _program Optional custom shader (null = use default)
void line(float _x1, float _y1, float _x2, float _y2, Shader* _program = nullptr);

/// Draw 2D line between two position vectors
/// @param _a Start position
/// @param _b End position
/// @param _program Optional custom shader (null = use default)
void line(const glm::vec2& _a, const glm::vec2& _b, Shader* _program = nullptr);

/// Draw 2D polyline (connected line segments)
/// @param _positions Array of vertex positions
/// @param _program Optional custom shader (null = use default)
void line(const std::vector<glm::vec2>& _positions, Shader* _program = nullptr);

/// Draw 3D line between two points
/// @param _x1 Start X coordinate
/// @param _y1 Start Y coordinate
/// @param _z1 Start Z coordinate
/// @param _x2 End X coordinate
/// @param _y2 End Y coordinate
/// @param _z2 End Z coordinate
/// @param _program Optional custom shader (null = use default)
void line(float _x1, float _y1, float _z1, float _x2, float _y2, float _z2, Shader* _program = nullptr);

/// Draw 3D line between two position vectors
/// @param _a Start position
/// @param _b End position
/// @param _program Optional custom shader (null = use default)
void line(const glm::vec3& _a, const glm::vec3& _b, Shader* _program = nullptr);

/// Draw 3D polyline (connected line segments)
/// @param _positions Array of vertex positions
/// @param _program Optional custom shader (null = use default)
void line(const std::vector<glm::vec3>& _positions, Shader* _program = nullptr);

/// Draw line from Line object
/// @param _line Line to draw
/// @param _program Optional custom shader (null = use default)
void line(const Line& _line, Shader* _program = nullptr);

/// Draw polyline from Polyline object
/// @param _polyline Polyline to draw
/// @param _program Optional custom shader (null = use default)
void line(const Polyline& _polyline, Shader* _program = nullptr);

/// Draw triangle wireframe (three connected lines)
/// @param _triangle Triangle to outline
/// @param _program Optional custom shader (null = use default)
void line(const Triangle& _triangle, Shader* _program = nullptr);

/// Draw bounding box wireframe
/// @param _bbox Bounding box to outline
/// @param _program Optional custom shader (null = use default)
void line(const BoundingBox& _bbox, Shader* _program = nullptr);

/// Draw 2D bounding box wireframe (vec4: x, y, w, h)
/// @param _bbox 2D bounding box (x, y, width, height)
/// @param _program Optional custom shader (null = use default)
void lineBoundingBox(const glm::vec4& _bbox, Shader* _program = nullptr);

/// Draw multiple 2D line segments as one batch (every consecutive pair of vertices = one independent segment)
/// @param _positions Array of vertex positions (must be even count; [0,1] = seg 0, [2,3] = seg 1, …)
/// @param _program Optional custom shader (null = use default)
void lines(const std::vector<glm::vec2>& _positions, Shader* _program = nullptr);

/// Draw an arc (portion of an ellipse)
/// @param _x Center X coordinate
/// @param _y Center Y coordinate
/// @param _w Arc width (ellipse diameter)
/// @param _h Arc height (ellipse diameter)
/// @param _start Start angle in radians
/// @param _stop End angle in radians
/// @param _mode Arc rendering mode (OPEN, CHORD, or PIE)
/// @param _program Optional custom shader (null = use default)
void arc(float _x, float _y, float _w, float _h, float _start, float _stop, ArcMode _mode = PIE_MODE, Shader* _program = nullptr);

// =============================================================================
// 2D SHAPE PRIMITIVES
// =============================================================================

/// Draw equilateral triangle centered at 2D position
/// @param _center Center position
/// @param angle Rotation angle in radians (default 0)
/// @param _radius Circumradius (distance from center to vertex, default 1)
/// @param _program Optional custom shader (null = use default)
void triangle(const glm::vec2& _center, float angle = 0.0, float _radius = 1.0,  Shader* _program = nullptr);

/// Draw equilateral triangle centered at 3D position
/// @param _center Center position
/// @param angle Rotation angle in radians (default 0)
/// @param _radius Circumradius (distance from center to vertex, default 1)
/// @param _program Optional custom shader (null = use default)
void triangle(const glm::vec3& _center, float angle = 0.0, float _radius = 1.0,  Shader* _program = nullptr);

/// Draw equilateral triangle centered at 3D position with custom orientation
/// @param _center Center position
/// @param _up Up vector defining triangle orientation
/// @param _radius Circumradius (distance from center to vertex, default 1)
/// @param _program Optional custom shader (null = use default)
void triangle(const glm::vec3& _center, glm::vec3 _up, float _radius = 1.0f, Shader* _program = nullptr);

/// Draw 2D triangle from three corner coordinates
/// @param _x1 First vertex X coordinate
/// @param _y1 First vertex Y coordinate
/// @param _x2 Second vertex X coordinate
/// @param _y2 Second vertex Y coordinate
/// @param _x3 Third vertex X coordinate
/// @param _y3 Third vertex Y coordinate
/// @param _program Optional custom shader (null = use default)
void triangle(float _x1, float _y1, float _x2, float _y2, float _x3, float _y3, Shader* _program = nullptr);

/// Draw 2D triangle from three corner vectors
/// @param _a First vertex position
/// @param _b Second vertex position
/// @param _c Third vertex position
/// @param _program Optional custom shader (null = use default)
void triangle(const glm::vec2& _a, const glm::vec2& _b, const glm::vec2& _c, Shader* _program = nullptr);

/// Draw 3D triangle from three corner coordinates
/// @param _x1 First vertex X coordinate
/// @param _y1 First vertex Y coordinate
/// @param _z1 First vertex Z coordinate
/// @param _x2 Second vertex X coordinate
/// @param _y2 Second vertex Y coordinate
/// @param _z2 Second vertex Z coordinate
/// @param _x3 Third vertex X coordinate
/// @param _y3 Third vertex Y coordinate
/// @param _z3 Third vertex Z coordinate
/// @param _program Optional custom shader (null = use default)
void triangle(float _x1, float _y1, float _z1, float _x2, float _y2, float _z2, float _x3, float _y3, float _z3, Shader* _program = nullptr);

/// Draw 3D triangle from three corner vectors
/// @param _a First vertex position
/// @param _b Second vertex position
/// @param _c Third vertex position
/// @param _program Optional custom shader (null = use default)
void triangle(const glm::vec3& _a, const glm::vec3& _b, const glm::vec3& _c, Shader* _program = nullptr);

/// Draw multiple 2D triangles from position array (every 3 vertices = 1 triangle)
/// @param _positions Array of vertex positions
/// @param _program Optional custom shader (null = use default)
void triangles(const std::vector<glm::vec2>& _positions, Shader* _program = nullptr);

/// Draw multiple 3D triangles from position array (every 3 vertices = 1 triangle)
/// @param _positions Array of vertex positions
/// @param _program Optional custom shader (null = use default)
void triangles(const std::vector<glm::vec3>& _positions, Shader* _program = nullptr);

/// Set horizontal alignment mode for subsequent rect() calls
/// @param _align Alignment mode (LEFT, CENTER, RIGHT)
void rectAlign(HorizontalAlign _align);

/// Set vertical alignment mode for subsequent rect() calls
/// @param _align Alignment mode (TOP, MIDDLE, BOTTOM)
void rectAlign(VerticalAlign _align);

/// Draw 2D rectangle
/// @param _x X position (meaning depends on rectAlign setting)
/// @param _y Y position (meaning depends on rectAlign setting)
/// @param _w Width
/// @param _h Height
/// @param _program Optional custom shader (null = use default)
void rect(float _x, float _y, float _w, float _h, Shader* _program = nullptr);

/// Draw 2D rectangle from position and size vectors
/// @param _pos Position (meaning depends on rectAlign setting)
/// @param _size Size (width, height)
/// @param _program Optional custom shader (null = use default)
void rect(const glm::vec2& _pos, const glm::vec2& _size, Shader* _program = nullptr);

/// Draw 2D rectangle from BoundingBox
/// @param _bbox Bounding box defining rectangle
/// @param _program Optional custom shader (null = use default)
void rect(const BoundingBox& _bbox, Shader* _program = nullptr);

/// Draw 2D quadrilateral from four corner coordinates
/// @param _x1 First vertex X coordinate
/// @param _y1 First vertex Y coordinate
/// @param _x2 Second vertex X coordinate
/// @param _y2 Second vertex Y coordinate
/// @param _x3 Third vertex X coordinate
/// @param _y3 Third vertex Y coordinate
/// @param _x4 Fourth vertex X coordinate
/// @param _y4 Fourth vertex Y coordinate
/// @param _program Optional custom shader (null = use default)
void quad(float _x1, float _y1, float _x2, float _y2, float _x3, float _y3, float _x4, float _y4, Shader* _program = nullptr);

/// Draw 2D quadrilateral from four corner vectors
/// @param _p1 First vertex position
/// @param _p2 Second vertex position
/// @param _p3 Third vertex position
/// @param _p4 Fourth vertex position
/// @param _program Optional custom shader (null = use default)
void quad(const glm::vec2& _p1, const glm::vec2& _p2, const glm::vec2& _p3, const glm::vec2& _p4, Shader* _program = nullptr);

/// Draw 3D quadrilateral from four corner coordinates
/// @param _x1 First vertex X coordinate
/// @param _y1 First vertex Y coordinate
/// @param _z1 First vertex Z coordinate
/// @param _x2 Second vertex X coordinate
/// @param _y2 Second vertex Y coordinate
/// @param _z2 Second vertex Z coordinate
/// @param _x3 Third vertex X coordinate
/// @param _y3 Third vertex Y coordinate
/// @param _z3 Third vertex Z coordinate
/// @param _x4 Fourth vertex X coordinate
/// @param _y4 Fourth vertex Y coordinate
/// @param _z4 Fourth vertex Z coordinate
/// @param _program Optional custom shader (null = use default)
void quad(float _x1, float _y1, float _z1, float _x2, float _y2, float _z2, float _x3, float _y3, float _z3, float _x4, float _y4, float _z4, Shader* _program = nullptr);

/// Draw 3D quadrilateral from four corner vectors
/// @param _p1 First vertex position
/// @param _p2 Second vertex position
/// @param _p3 Third vertex position
/// @param _p4 Fourth vertex position
/// @param _program Optional custom shader (null = use default)
void quad(const glm::vec3& _p1, const glm::vec3& _p2, const glm::vec3& _p3, const glm::vec3& _p4, Shader* _program = nullptr);

/// Draw 2D square centered at position
/// @param _x Center X coordinate
/// @param _y Center Y coordinate
/// @param _s Side length
/// @param _program Optional custom shader (null = use default)
void square(float _x, float _y, float _s, Shader* _program = nullptr);

/// Set circle/ellipse tessellation resolution
/// @param _res Number of line segments (default CIRCLE_RESOLUTION = 36)
void circleResolution(int _res = CIRCLE_RESOLUTION);

/// Draw 2D circle
/// @param _x Center X coordinate
/// @param _y Center Y coordinate
/// @param _r Radius
/// @param _program Optional custom shader (null = use default)
void circle(float _x, float _y, float _r, Shader* _program = nullptr);

/// Draw 2D circle from position vector
/// @param _pos Center position
/// @param _r Radius
/// @param _program Optional custom shader (null = use default)
void circle(const glm::vec2& _pos, float _r, Shader* _program = nullptr);

/// Draw 2D ellipse
/// @param _x Center X coordinate
/// @param _y Center Y coordinate
/// @param _w Width (horizontal diameter)
/// @param _h Height (vertical diameter)
/// @param _program Optional custom shader (null = use default)
void ellipse(float _x, float _y, float _w, float _h, Shader* _program = nullptr);

/// Draw 2D ellipse from position vector
/// @param _pos Center position
/// @param _w Width (horizontal diameter)
/// @param _h Height (vertical diameter)
/// @param _program Optional custom shader (null = use default)
void ellipse(const glm::vec2& _pos, float _w, float _h, Shader* _program = nullptr);

/// Get current horizontal rectangle alignment mode
/// @return Horizontal alignment setting
HorizontalAlign rectHorizontalAlign();

/// Get current vertical rectangle alignment mode
/// @return Vertical alignment setting
VerticalAlign   rectVerticalAlign();

// =============================================================================
// 3D SHAPE PRIMITIVES
// =============================================================================

/// Draw unit plane (1x1) in XY plane centered at origin
/// @param _program Optional custom shader (null = use default)
void plane(Shader* _program = nullptr);

/// Draw square plane centered at origin
/// @param _size Side length (creates _size x _size plane)
/// @param _program Optional custom shader (null = use default)
void plane(float _size, Shader* _program = nullptr);

/// Draw rectangular plane centered at origin
/// @param _width Width (X dimension)
/// @param _height Height (Y dimension)
/// @param _program Optional custom shader (null = use default)
void plane(float _width, float _height, Shader* _program = nullptr);

/// Draw rectangular plane with specified uniform tessellation
/// @param _width Width (X dimension)
/// @param _height Height (Y dimension)
/// @param _res Number of subdivisions in both dimensions
/// @param _program Optional custom shader (null = use default)
void plane(float _width, float _height, int _res, Shader* _program = nullptr);

/// Draw rectangular plane with specified non-uniform tessellation
/// @param _width Width (X dimension)
/// @param _height Height (Y dimension)
/// @param _resX Number of subdivisions in X direction
/// @param _resY Number of subdivisions in Y direction
/// @param _program Optional custom shader (null = use default)
void plane(float _width, float _height, int _resX, int _resY, Shader* _program = nullptr);

/// Draw unit cube (1x1x1) centered at origin
/// @param _program Optional custom shader (null = use default)
void box(Shader* _program = nullptr);

/// Draw cubic box centered at origin
/// @param _size Side length (creates _size x _size x _size cube)
/// @param _program Optional custom shader (null = use default)
void box(float _size, Shader* _program = nullptr);

/// Draw rectangular box centered at origin with same width and height
/// @param _width Width and height (X and Y dimensions)
/// @param _height Depth (Z dimension)
/// @param _program Optional custom shader (null = use default)
void box(float _width, float _height, Shader* _program = nullptr);

/// Draw rectangular box centered at origin
/// @param _width Width (X dimension)
/// @param _height Height (Y dimension)
/// @param _depth Depth (Z dimension)
/// @param _program Optional custom shader (null = use default)
void box(float _width, float _height, float _depth, Shader* _program = nullptr);

/// Draw unit sphere (radius 1) centered at origin
/// @param _program Optional custom shader (null = use default)
void sphere(Shader* _program = nullptr);

/// Draw sphere centered at origin  
/// @param _radius Sphere radius
/// @param _program Optional custom shader (null = use default)
void sphere(float _radius, Shader* _program = nullptr);

/// Draw sphere with specified tessellation
/// @param _radius Sphere radius
/// @param _res Tessellation resolution (higher = smoother)
/// @param _program Optional custom shader (null = use default)
void sphere(float _radius, int _res, Shader* _program = nullptr);

// cylinder()
// cone()
// ellipsoid()
// torus()
// p5.Geometry

// =============================================================================
// IMAGE AND TEXTURE RENDERING
// =============================================================================

/// Load image from file path
/// @param _name File path to load
/// @return Loaded Image object
Image loadImage(const std::string& _name);

/// Draw image from file path (full screen)
/// @param _path File path to image
void image(const std::string &_path);

/// Draw image from file path at position
/// @param _path File path to image
/// @param _x X position
/// @param _y Y position
/// @param _width Width (0 = use image width)
/// @param _height Height (0 = use image height)
void image(const std::string &_path, float _x, float _y, float _width = 0, float _height = 0);

/// Draw Image object (full screen)
/// @param _img Image to draw
void image(const Image &_img);

/// Draw Image pointer (full screen)
/// @param _img Pointer to Image to draw
void image(const Image *_img);

/// Draw Image object at position
/// @param _img Image to draw
/// @param _x X position
/// @param _y Y position
/// @param _width Width (0 = use image width)
/// @param _height Height (0 = use image height)
void image(const Image &_img, float _x, float _y, float _width = 0, float _height = 0);

/// Draw Image pointer at position
/// @param _img Pointer to Image to draw
/// @param _x X position
/// @param _y Y position
/// @param _width Width (0 = use image width)
/// @param _height Height (0 = use image height)
void image(const Image *_img, float _x, float _y, float _width = 0, float _height = 0);

/// Draw Texture object (full screen)
/// @param _tex Texture to draw
void image(const Texture &_tex);

/// Draw Texture pointer (full screen)
/// @param _tex Pointer to Texture to draw
void image(const Texture *_tex);

/// Draw Texture object at position
/// @param _tex Texture to draw
/// @param _x X position
/// @param _y Y position
/// @param _width Width (0 = use texture width)
/// @param _height Height (0 = use texture height)
void image(const Texture &_tex, float _x, float _y, float _width = 0, float _height = 0);

/// Draw Texture pointer at position
/// @param _tex Pointer to Texture to draw
/// @param _x X position
/// @param _y Y position
/// @param _width Width (0 = use texture width)
/// @param _height Height (0 = use texture height)
void image(const Texture *_tex, float _x, float _y, float _width = 0, float _height = 0);

/// Draw TextureStream object (full screen)
/// @param _stream Texture stream to draw
void image(const TextureStream &_stream);

/// Draw TextureStream pointer (full screen)
/// @param _stream Pointer to texture stream to draw
void image(const TextureStream *_stream);

/// Draw TextureStream object at position
/// @param _stream Texture stream to draw
/// @param _x X position
/// @param _y Y position
/// @param _width Width (0 = use stream width)
/// @param _height Height (0 = use stream height)
/// @param _debug Enable debug visualization
void image(const TextureStream &_stream, float _x, float _y, float _width = 0, float _height = 0, bool _debug = false);

/// Draw TextureStream pointer at position
/// @param _stream Pointer to texture stream to draw
/// @param _x X position
/// @param _y Y position
/// @param _width Width (0 = use stream width)
/// @param _height Height (0 = use stream height)
/// @param _debug Enable debug visualization
void image(const TextureStream *_stream, float _x, float _y, float _width = 0, float _height = 0, bool _debug = false);

/// Draw Fbo (framebuffer object) color attachment (full screen)
/// @param _fbo Framebuffer to draw
void image(const Fbo &_fbo);

/// Draw Fbo pointer color attachment (full screen)
/// @param _fbo Pointer to framebuffer to draw
void image(const Fbo *_fbo);

/// Draw Fbo color attachment at position
/// @param _fbo Framebuffer to draw
/// @param _x X position
/// @param _y Y position
/// @param _width Width (0 = use fbo width)
/// @param _height Height (0 = use fbo height)
void image(const Fbo &_fbo, float _x, float _y, float _width = 0, float _height = 0);

/// Draw Fbo color attachment with custom world matrix
/// @param _fbo Framebuffer to draw
/// @param _x X position
/// @param _y Y position
/// @param _width Width
/// @param _height Height
/// @param _world World transformation matrix
void image(const Fbo &_fbo, float _x, float _y, float _width, float _height, glm::mat4 _world);

/// Draw Fbo pointer color attachment at position
/// @param _fbo Pointer to framebuffer to draw
/// @param _x X position
/// @param _y Y position
/// @param _width Width (0 = use fbo width)
/// @param _height Height (0 = use fbo height)
void image(const Fbo *_fbo, float _x, float _y, float _width = 0, float _height = 0);

/// Draw Fbo pointer color attachment with custom world matrix
/// @param _fbo Pointer to framebuffer to draw
/// @param _x X position
/// @param _y Y position
/// @param _width Width
/// @param _height Height
/// @param _world World transformation matrix
void image(const Fbo *_fbo, float _x, float _y, float _width, float _height, glm::mat4 _world);

/// Draw Fbo depth buffer at position (visualized)
/// @param _fbo Framebuffer to draw depth from
/// @param _x X position
/// @param _y Y position
/// @param _width Width (0 = use fbo width)
/// @param _height Height (0 = use fbo height)
/// @param _far Far plane distance for depth linearization
/// @param _near Near plane distance for depth linearization
void imageDepth(const Fbo &_fbo, float _x, float _y, float _width = 0, float _height = 0, float _far = 100.0f, float _near = 0.001f);

/// Draw Fbo pointer depth buffer at position (visualized)
/// @param _fbo Pointer to framebuffer to draw depth from
/// @param _x X position
/// @param _y Y position
/// @param _width Width (0 = use fbo width)
/// @param _height Height (0 = use fbo height)
/// @param _far Far plane distance for depth linearization
/// @param _near Near plane distance for depth linearization
void imageDepth(const Fbo *_fbo, float _x, float _y, float _width = 0, float _height = 0, float _far = 100.0f, float _near = 0.001f);

/// Get VBO for billboard rendering (internal)
/// @return Pointer to billboard VBO
Vbo* billboard();

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

// =============================================================================
// FONT MANAGEMENT
// =============================================================================

/// Load font from file
/// @param _file Font file path (e.g., TTF file)
/// @return Pointer to loaded Font object
Font* loadFont(const std::string& _file);

/// Add font to font registry with reference
/// @param _name Font identifier name
/// @param _font Font object reference
void  addFont(const std::string _name, Font& _font);

/// Add font to font registry with pointer
/// @param _name Font identifier name
/// @param _font Pointer to Font object
void  addFont(const std::string _name, Font* _font);

/// Load and add font to registry from file
/// @param _name Font identifier name
/// @param _file Font file path
void  addFont(const std::string _name, const std::string& _file);

/// Get current default font
/// @return Pointer to current Font
Font* font();

/// Get font by name from registry
/// @param _name Font identifier name
/// @return Pointer to Font object, or null if not found
Font* font(const std::string& _name);

// =============================================================================
// TEXT RENDERING
// =============================================================================

// textLeading()
// textStyle()
// textAscent()
// textDescent()
// textWrap()

/// Set text font by name from registry
/// @param _name Font identifier name
/// @return Pointer to Font object
Font* textFont(const std::string& _name);

/// Set text font by name and size
/// @param _name Font identifier name
/// @param _size Text size
/// @return Pointer to Font object
Font* textFont(const std::string& _name, float _size);

/// Set horizontal text alignment
/// @param _align Horizontal alignment (LEFT, CENTER, RIGHT)
/// @param _font Font to apply to (null = current font)
void  textAlign(HorizontalAlign _align, Font* _font = nullptr);

/// Set vertical text alignment
/// @param _align Vertical alignment (TOP, MIDDLE, BOTTOM)
/// @param _font Font to apply to (null = current font)
void  textAlign(VerticalAlign _align, Font* _font = nullptr);

/// Set text rotation angle
/// @param _angle Rotation angle in radians
/// @param _font Font to apply to (null = current font)
void  textAngle(float _angle, Font* _font = nullptr);

/// Set text size
/// @param _size Text size
/// @param _font Font to apply to (null = current font)
void  textSize(float _size, Font* _font = nullptr);

/// Calculate rendered width of text string
/// @param _text Text to measure
/// @param _font Font to use (null = current font)
/// @return Width in pixels
float textWidth(const std::string& _text, Font* _font = nullptr);

/// Get current text height (line height)
/// @param _font Font to use (null = current font)
/// @return Height in pixels
float textHeight(Font* _font = nullptr);

/// Calculate required height for text string
/// @param _text Text to measure
/// @param _font Font to use (null = current font)
/// @return Height in pixels
float textHeight(const std::string& _text, Font* _font = nullptr);

/// Calculate bounding box for text at position
/// @param _text Text to measure
/// @param _x X position
/// @param _y Y position
/// @param _font Font to use (null = current font)
/// @return Bounding box of rendered text
BoundingBox textBoundingBox(const std::string& _text, float _x, float _y, Font* _font = nullptr);

/// Calculate bounding box for text at 2D position
/// @param _text Text to measure
/// @param _pos Position
/// @param _font Font to use (null = current font)
/// @return Bounding box of rendered text
BoundingBox textBoundingBox(const std::string& _text, const glm::vec2& _pos, Font* _font = nullptr);

/// Calculate bounding box for text at 3D position
/// @param _text Text to measure
/// @param _pos Position
/// @param _font Font to use (null = current font)
/// @return Bounding box of rendered text
BoundingBox textBoundingBox(const std::string& _text, const glm::vec3& _pos, Font* _font = nullptr);

/// Draw text at 2D position
/// @param _text Text to draw
/// @param _pos Position
/// @param _font Font to use (null = current font)
void text(const std::string& _text, const glm::vec2& _pos, Font* _font = nullptr );

/// Draw text at 3D position
/// @param _text Text to draw
/// @param _pos Position
/// @param _font Font to use (null = current font)
void text(const std::string& _text, const glm::vec3& _pos, Font* _font = nullptr);

/// Draw text at position
/// @param _text Text to draw
/// @param _x X position
/// @param _y Y position
/// @param _font Font to use (null = current font)
void text(const std::string& _text, float _x, float _y, Font* _font = nullptr);

/// Draw text with background highlight at 2D position
/// @param _text Text to draw
/// @param _pos Position
/// @param _bg Background color (default black)
/// @param _font Font to use (null = current font)
void textHighlight(const std::string& _text, const glm::vec2& _pos, const glm::vec4& _bg = glm::vec4(0.0, 0.0, 0.0, 1.0), Font* _font = nullptr);

/// Draw text with background highlight at position
/// @param _text Text to draw
/// @param _x X position
/// @param _y Y position
/// @param _bg Background color (default black)
/// @param _font Font to use (null = current font)
void textHighlight(const std::string& _text, float _x, float _y, const glm::vec4& _bg = glm::vec4(0.0, 0.0, 0.0, 1.0), Font* _font = nullptr);

// =============================================================================
// SHADER MANAGEMENT
// =============================================================================

/// Load shader from fragment and vertex files
/// @param _fragFile Fragment shader file path
/// @param _vertFile Vertex shader file path
/// @return Pointer to loaded Shader
Shader* loadShader(const std::string& _fragFile, const std::string& _vertFile);

/// Create shader from source strings
/// @param _fragSrc Fragment shader source code
/// @param _vertSrc Vertex shader source code
/// @return Pointer to created Shader
Shader* createShader(const std::string& _fragSrc = "", const std::string& _vertSrc = "");

/// Create shader from default shader enums
/// @param _frag Default fragment shader type
/// @param _vert Default vertex shader type
/// @return Pointer to created Shader
Shader* createShader(DefaultShaders _frag, DefaultShaders _vert);

/// Add shader to registry with reference
/// @param _name Shader identifier name
/// @param _shader Shader object reference
/// @return Pointer to added Shader
Shader* addShader(const std::string& _name, Shader& _shader);

/// Add shader to registry with pointer
/// @param _name Shader identifier name
/// @param _shader Pointer to Shader object
/// @return Pointer to added Shader
Shader* addShader(const std::string& _name, Shader* _shader);

/// Create and add shader to registry from source
/// @param _name Shader identifier name
/// @param _fragSrc Fragment shader source code
/// @param _vertSrc Vertex shader source code
/// @return Pointer to created Shader
Shader* addShader(const std::string& _name, const std::string& _fragSrc = "", const std::string& _vertSrc = "");

/// Get list of all shader names in registry
/// @return Vector of shader names
std::vector<std::string> listShaders();

/// Reset to default shader
void    resetShader();

/// Set active shader from reference
/// @param _shader Shader object reference
/// @return Pointer to active Shader
Shader* shader(Shader& _shader);

/// Set active shader from pointer
/// @param _shader Pointer to Shader object
/// @return Pointer to active Shader
Shader* shader(Shader* _shader);

/// Set active shader by name from registry
/// @param _name Shader identifier name
/// @return Pointer to active Shader
Shader* shader(const std::string& _name);

/// Get current active shader
/// @return Pointer to active Shader
Shader* shader();

/// Get shader used for point rendering
/// @return Pointer to point Shader
Shader* pointShader();

/// Get shader used for stroke (line) rendering
/// @return Pointer to stroke Shader
Shader* strokeShader();

/// Get shader used for 2D spline rendering
/// @return Pointer to 2D spline Shader
Shader* spline2DShader();

/// Get shader used for 3D spline rendering
/// @return Pointer to 3D spline Shader
Shader* spline3DShader();

/// Get shader used for fill rendering
/// @return Pointer to fill Shader
Shader* fillShader();

// =============================================================================
// TEXTURE MANAGEMENT
// =============================================================================

/// Load and add texture to registry from file
/// @param _name Texture identifier name
/// @param _filename Image file path
/// @param _vFlip Vertical flip on load (default false)
/// @param _filter Texture filtering mode (default LINEAR)
/// @param _wrap Texture wrap mode (default REPEAT)
void    addTexture(const std::string& _name, const std::string& _filename, bool _vFlip = false, TextureFilter _filter = LINEAR, TextureWrap _wrap = REPEAT);

/// Add texture to registry from Image object
/// @param _name Texture identifier name
/// @param _image Image object to create texture from
/// @param _filter Texture filtering mode (default LINEAR)
/// @param _wrap Texture wrap mode (default REPEAT)
void    addTexture(const std::string& _name, const vera::Image& _image, TextureFilter _filter = LINEAR, TextureWrap _wrap = REPEAT);

/// Get texture by name from registry
/// @param _name Texture identifier name
/// @return Pointer to Texture, or null if not found
Texture* texture(const std::string _name);

/// Get texture and bind to shader uniform
/// @param _name Texture identifier name
/// @param _uniform_name Shader uniform name to bind to
/// @return Pointer to Texture
Texture* texture(const std::string _name, const std::string _uniform_name);

/// Bind texture to current shader
/// @param _texture Texture object reference
/// @param _uniform_name Shader uniform name (default empty)
/// @return Pointer to Texture
Texture* texture(Texture& _texture, const std::string _uniform_name = "");

/// Bind texture pointer to current shader
/// @param _texture Pointer to Texture object
/// @param _uniform_name Shader uniform name (default empty)
/// @return Pointer to Texture
Texture* texture(Texture* _texture, const std::string _uniform_name = "");

// texture(Image& _image);
// texture(Image* _image);
// textire(Fbo& _fbo);
// texture(Fbo* _fbo);
// texture(TextureStream& _stream);
// texture(TextureStream* _stream); 

// textureMode()
// textureWrap()

// Interaction
// -----------------------------
// debugMode()
// noDebugMode()

// =============================================================================
// SCENE MANAGEMENT
// =============================================================================

/// Set active scene from reference
/// @param scene Scene object reference
void scene(Scene& scene);

/// Set active scene from pointer
/// @param scene Pointer to Scene object
void scene(Scene* scene);

/// Get current active scene
/// @return Pointer to active Scene
Scene* scene();

// =============================================================================
// CAMERA MANAGEMENT
// =============================================================================

// camera()

/// Set perspective projection matrix
/// @param _fovy Field of view in Y direction (radians)
/// @param _aspect Aspect ratio (width/height)
/// @param _near Near clipping plane distance
/// @param _far Far clipping plane distance
void perspective(float _fovy, float _aspect, float _near, float _far);

/// Set orthographic projection matrix
/// @param _left Left clipping plane
/// @param _right Right clipping plane
/// @param _bottom Bottom clipping plane
/// @param _top Top clipping plane
/// @param _near Near clipping plane distance
/// @param _far Far clipping plane distance
void ortho(float _left, float _right, float _bottom, float _top,  float _near, float _far);

// frustum()

/// Create new Camera object
/// @return Pointer to new Camera
Camera* createCamera();

/// Add camera to registry with reference
/// @param _name Camera identifier name
/// @param _camera Camera object reference
void addCamera(const std::string& _name, Camera& _camera);

/// Add camera to registry with pointer
/// @param _name Camera identifier name
/// @param _camera Pointer to Camera object
void addCamera(const std::string& _name, Camera* _camera);

/// Set active camera by name from registry
/// @param _name Camera identifier name
void setCamera(const std::string& _name);

/// Set active camera from reference
/// @param _camera Camera object reference
void setCamera(Camera& _camera);

/// Set active camera from pointer
/// @param _camera Pointer to Camera object
void setCamera(Camera* _camera);

/// Reset to default camera
void resetCamera();

/// Get current active camera
/// @return Pointer to active Camera
Camera* camera();

/// Get previously active camera
/// @return Pointer to last Camera
Camera* cameraLast();

/// Get camera by name from registry
/// @param _name Camera identifier name
/// @return Pointer to Camera, or null if not found
Camera* camera(const std::string& _name);

// =============================================================================
// LIGHTING
// =============================================================================

// ambientLight()
// specularColor()
// directionalLight()
// pointLight()
// lightFalloff()
// spotLight()

/// Enable lighting calculations
void lights();

/// Disable lighting calculations
void noLights();

/// Create new Light object
/// @return Pointer to new Light
Light* createLight();

/// Add light to registry with reference
/// @param _name Light identifier name
/// @param _light Light object reference
void addLight(const std::string& _name, Light& _light);

/// Add light to registry with pointer
/// @param _name Light identifier name
/// @param _light Pointer to Light object
void addLight(const std::string& _name, Light* _light);

// =============================================================================
// MATERIALS
// =============================================================================

// normalMaterial()
// ambientMaterial()
// emissiveMaterial()
// specularMaterial()
// shininess()

// =============================================================================
// 3D MODEL RENDERING
// =============================================================================

// void loadModel( const std::string& _filename );

/// Render VBO (vertex buffer object) with reference
/// @param _vbo VBO object reference
/// @param _program Optional custom shader (null = use default)
void model(Vbo& _vbo, Shader* _program = nullptr);

/// Render VBO (vertex buffer object) with pointer
/// @param _vbo Pointer to VBO object
/// @param _program Optional custom shader (null = use default)
void model(Vbo* _vbo, Shader* _program = nullptr);

/// Render mesh object
/// @param _mesh Mesh object to render
/// @param _program Optional custom shader (null = use default)
void model(const Mesh& _mesh, Shader* _program = nullptr);

#ifdef SUPPORT_GSPLAT
/// Render Gaussian splat object
/// @param _gsplat Gaussian splat object to render
/// @param _program Optional custom shader (null = use default)
void model(Gsplat& _gsplat, Shader* _program = nullptr);
#endif

// =============================================================================
// LABEL RENDERING (3D text annotations that face camera)
// =============================================================================

/// Add label object with reference
/// @param _label Label object reference
void addLabel(Label& _label);

/// Add label object with pointer
/// @param _label Pointer to Label object
void addLabel(Label* _label);

// Ephemeral labels (copy position, not anchored)

/// Add ephemeral label at 3D position
/// @param _text Label text (C-string)
/// @param _position 3D position
/// @param _type Label placement type (default LABEL_CENTER)
/// @param _margin Distance from position in pixels (default 0)
void addLabel(const char* _text, glm::vec3 _position, LabelType _type = LABEL_CENTER, float _margin = 0.0f);

/// Add ephemeral label at 3D position
/// @param _text Label text (std::string)
/// @param _position 3D position
/// @param _type Label placement type (default LABEL_CENTER)
/// @param _margin Distance from position in pixels (default 0)
void addLabel(const std::string& _text, glm::vec3 _position, LabelType _type = LABEL_CENTER, float _margin = 0.0f);

// Anchor labels (track pointer/node, position updates dynamically)

/// Add label anchored to 3D position pointer
/// @param _text Label text (C-string)
/// @param _position Pointer to 3D position (updated each frame)
/// @param _type Label placement type (default LABEL_CENTER)
/// @param _margin Distance from position in pixels (default 0)
void addLabel(const char* _text, glm::vec3* _position, LabelType _type = LABEL_CENTER, float _margin = 0.0f);

/// Add label anchored to Node object
/// @param _text Label text (C-string)
/// @param _node Pointer to Node to track
/// @param _type Label placement type (default LABEL_CENTER)
/// @param _margin Distance from position in pixels (default 0)
void addLabel(const char* _text, Node* _node, LabelType _type = LABEL_CENTER, float _margin = 0.0f);

/// Add label anchored to Model object
/// @param _text Label text (C-string)
/// @param _node Pointer to Model to track
/// @param _type Label placement type (default LABEL_CENTER)
/// @param _margin Distance from position in pixels (default 0)
void addLabel(const char* _text, Model* _node, LabelType _type = LABEL_CENTER, float _margin = 0.0f);

/// Add label anchored to 3D position pointer
/// @param _text Label text (std::string)
/// @param _position Pointer to 3D position (updated each frame)
/// @param _type Label placement type (default LABEL_CENTER)
/// @param _margin Distance from position in pixels (default 0)
void addLabel(const std::string& _text, glm::vec3* _position, LabelType _type = LABEL_CENTER, float _margin = 0.0f);

/// Add label anchored to Node object
/// @param _text Label text (std::string)
/// @param _node Pointer to Node to track
/// @param _type Label placement type (default LABEL_CENTER)
/// @param _margin Distance from position in pixels (default 0)
void addLabel(const std::string& _text, Node* _node, LabelType _type = LABEL_CENTER, float _margin = 0.0f);

/// Add label anchored to Model object
/// @param _text Label text (std::string)
/// @param _node Pointer to Model to track
/// @param _type Label placement type (default LABEL_CENTER)
/// @param _margin Distance from position in pixels (default 0)
void addLabel(const std::string& _text, Model* _node, LabelType _type = LABEL_CENTER, float _margin = 0.0f);

/// Add dynamic label with text from function, anchored to 3D position
/// @param _func Function returning text string (called each frame)
/// @param _position Pointer to 3D position (updated each frame)
/// @param _type Label placement type (default LABEL_CENTER)
/// @param _margin Distance from position in pixels (default 0)
void addLabel(std::function<std::string(void)> _func, glm::vec3* _position, LabelType _type = LABEL_CENTER, float _margin = 0.0f);

/// Add dynamic label with text from function, anchored to Node
/// @param _func Function returning text string (called each frame)
/// @param _node Pointer to Node to track
/// @param _type Label placement type (default LABEL_CENTER)
/// @param _margin Distance from position in pixels (default 0)
void addLabel(std::function<std::string(void)> _func, Node* _node, LabelType _type = LABEL_CENTER, float _margin = 0.0f);

/// Add dynamic label with text from function, anchored to Model
/// @param _func Function returning text string (called each frame)
/// @param _model Pointer to Model to track
/// @param _type Label placement type (default LABEL_CENTER)
/// @param _margin Distance from position in pixels (default 0)
void addLabel(std::function<std::string(void)> _func, Model* _model, LabelType _type = LABEL_CENTER, float _margin = 0.0f);

/// Render all labels (called automatically each frame)
void labels();

/// Remove all labels
void cleanLabels();

/// Get global label settings object
/// @return Reference to LabelSettings
LabelSettings& labelSettings();

/// Set center point for radial label layout
/// @param _x Center X coordinate
/// @param _y Center Y coordinate
void labelRadialCenter(float _x, float _y);

/// Set center point for radial label layout
/// @param _center 2D center position
void labelRadialCenter(const glm::vec2& _center);

/// Set center point for radial label layout
/// @param _center 3D center position
void labelRadialCenter(const glm::vec3& _center);

/// Get index of label at screen coordinates (for picking)
/// @param _x Screen X coordinate
/// @param _y Screen Y coordinate
/// @return Label index, or -1 if none at position
int labelAt(float _x, float _y);

/// Get label by index
/// @param _index Label index
/// @return Pointer to Label, or null if invalid index
Label* label(size_t _index);

};
