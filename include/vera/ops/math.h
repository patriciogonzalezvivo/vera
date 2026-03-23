#pragma once

#include <math.h>
#include <iostream>
#include "glm/glm.hpp"

namespace vera {

// =============================================================================
// MATHEMATICAL CONSTANTS
// =============================================================================

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif 

#ifndef M_2_SQRTPI
#define M_2_SQRTPI 1.12837916709551257390
#endif

#ifndef M_MIN
#define M_MIN(_a, _b) ((_a)<(_b)?(_a):(_b))
#endif

#ifndef PI
#define PI 3.14159265358979323846
#endif 

#ifndef TWO_PI
#define TWO_PI   6.28318530717958647693
#endif

#ifndef FOUR_PI
#define FOUR_PI 12.56637061435917295385
#endif

#ifndef HALF_PI
#define HALF_PI  1.57079632679489661923
#endif

#ifndef QUARTER_PI
#define QUARTER_PI 0.785398163
#endif

#ifndef FLT_EPSILON
#define FLT_EPSILON 1.19209290E-07F
#endif

#ifndef MIN
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef CLAMP
#define CLAMP(val,min,max) ((val) < (min) ? (min) : ((val > max) ? (max) : (val)))
#endif

#ifndef ABS
#define ABS(x) (((x) < 0) ? -(x) : (x))
#endif

// =============================================================================
// BASIC MATHEMATICAL FUNCTIONS
// =============================================================================

/// Calculate the angle between two points in spherical coordinates
/// @param thetav Theta angle of the view vector
/// @param phiv Phi angle of the view vector
/// @param theta Theta angle of the target point
/// @param phi Phi angle of the target point
/// @return The angle in radians between the two vectors
float angleBetween(float thetav, float phiv, float theta, float phi);

/// Clamp a value to the range [0, 1]
/// @param value The value to clamp
/// @return The clamped value between 0.0 and 1.0
float saturate(float value);

/// Compute the square of a value
/// @tparam T Numeric type
/// @param x The value to square
/// @return x * x
template <class T>
T square(const T &x) { return x*x; }

/// Get the sign of a value
/// @tparam T Numeric type
/// @param x The value to check
/// @return 1 if x >= 0, otherwise -1
template <class T>
int sign(const T &x) { return (x >= 0 ? 1 : -1); }

/// Clamp a value to a specified range
/// @param value The value to clamp
/// @param min The minimum bound
/// @param max The maximum bound
/// @return The clamped value
float clamp ( float value , float min , float max );

/// Round a number to the nearest multiple of a factor
/// @param num The number to round
/// @param factor The factor to round to
/// @return The rounded value
int roundTo(int num, int factor);

// =============================================================================
// INTERPOLATION AND REMAPPING
// =============================================================================

/// Linear interpolation between two values
/// @param a The start value
/// @param b The end value
/// @param f The interpolation factor [0, 1]
/// @return The interpolated value: a + (b - a) * f
float lerp(float a, float b, float f); 

/// Remap a value from one range to another
/// @param _value The value to remap
/// @param _inputMin The minimum of the input range
/// @param _inputMax The maximum of the input range
/// @param _outputMin The minimum of the output range
/// @param _outputMax The maximum of the output range
/// @param _clamp Whether to clamp the result to the output range
/// @return The remapped value
float remap(const float& _value, const float& _inputMin, const float& _inputMax, const float& _outputMin, const float& _outputMax, bool _clamp);

/// Remap a 2D vector from one range to another (component-wise)
/// @param _value The vector to remap
/// @param _inputMin The minimum of the input range
/// @param _inputMax The maximum of the input range
/// @param _outputMin The minimum of the output range
/// @param _outputMax The maximum of the output range
/// @param _clamp Whether to clamp the result to the output range
/// @return The remapped vector
glm::vec2 remap(const glm::vec2& _value, const glm::vec2& _inputMin, const glm::vec2& _inputMax, const glm::vec2& _outputMin, const glm::vec2& _outputMax, bool _clamp);

/// Remap a 3D vector from one range to another (component-wise)
/// @param _value The vector to remap
/// @param _inputMin The minimum of the input range
/// @param _inputMax The maximum of the input range
/// @param _outputMin The minimum of the output range
/// @param _outputMax The maximum of the output range
/// @param _clamp Whether to clamp the result to the output range
/// @return The remapped vector
glm::vec3 remap(const glm::vec3& _value, const glm::vec3& _inputMin, const glm::vec3& _inputMax, const glm::vec3& _outputMin, const glm::vec3& _outputMax, bool _clamp);

/// Remap a 4D vector from one range to another (component-wise)
/// @param _value The vector to remap
/// @param _inputMin The minimum of the input range
/// @param _inputMax The maximum of the input range
/// @param _outputMin The minimum of the output range
/// @param _outputMax The maximum of the output range
/// @param _clamp Whether to clamp the result to the output range
/// @return The remapped vector
glm::vec4 remap(const glm::vec4& _value, const glm::vec4& _inputMin, const glm::vec4& _inputMax, const glm::vec4& _outputMin, const glm::vec4& _outputMax, bool _clamp);

// =============================================================================
// MATRIX OPERATIONS
// =============================================================================

/// Compute the inverse of a 2x2 matrix
/// @param m The matrix to invert
/// @return The inverse matrix
glm::mat2 inverseMatrix(const glm::mat2& m);

/// Compute the inverse of a 3x3 matrix
/// @param m The matrix to invert
/// @return The inverse matrix
glm::mat3 inverseMatrix(const glm::mat3& m);

/// Compute the inverse of a 4x4 matrix
/// @param m The matrix to invert
/// @return The inverse matrix
glm::mat4 inverseMatrix(const glm::mat4& m);

// =============================================================================
// RANDOM NUMBER GENERATION
// =============================================================================

/// Set the seed value for random number generation
void randomSeed(int seed);

/// Generate a random float in the range [0, 1]
float random();

/// Generate a random float in the range [min, max]
float random(float min, float max);

/// Generate a random integer in the range [min, max]
int random(int min, int max);

/// Generate a random number from a Gaussian (normal) distribution
/// @param mean The mean (center) of the distribution (default: 0.0)
/// @param sd The standard deviation (spread) of the distribution (default: 1.0)
/// @return A random value following a Gaussian distribution
/// @note Uses Box-Muller transform following P5.js API specification
float randomGaussian(float mean = 0.0f, float sd = 1.0f);

// =============================================================================
// PERLIN NOISE (P5.js compatible API)
// =============================================================================

/// Set the seed value for Perlin noise generation
/// @param seed Random seed for reproducible noise patterns
void noiseSeed(unsigned int seed);

/// Adjust the character and level of detail produced by noise()
/// @param lod Number of octaves to be used by noise (default: 4)
/// @param falloff Falloff factor for each octave (default: 0.5)
void noiseDetail(int lod, float falloff = 0.5f);

/// Generate 1D Perlin noise
/// @param x The x-coordinate in noise space
/// @return A noise value between 0.0 and 1.0
float noise(float x);

/// Generate 2D Perlin noise
/// @param x The x-coordinate in noise space
/// @param y The y-coordinate in noise space
/// @return A noise value between 0.0 and 1.0
float noise(float x, float y);

/// Generate 3D Perlin noise
/// @param x The x-coordinate in noise space
/// @param y The y-coordinate in noise space
/// @param z The z-coordinate in noise space
/// @return A noise value between 0.0 and 1.0
float noise(float x, float y, float z);

/// Generate 2D Perlin noise from a vector
/// @param v The 2D coordinate in noise space
/// @return A noise value between 0.0 and 1.0
float noise(const glm::vec2& v);

/// Generate 3D Perlin noise from a vector
/// @param v The 3D coordinate in noise space
/// @return A noise value between 0.0 and 1.0
float noise(const glm::vec3& v);

} // namespace vera