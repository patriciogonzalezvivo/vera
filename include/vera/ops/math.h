#pragma once

#include <math.h>
#include <iostream>
#include "glm/glm.hpp"

namespace vera {

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

static float angleBetween(float thetav, float phiv, float theta, float phi) {
    float cosGamma = sinf(thetav) * sinf(theta) * cosf(phi - phiv) + cosf(thetav) * cosf(theta);
    return acosf(cosGamma);
}

inline float saturate(float value) { 
    return std::fmax (0.0f, std::fmin (1.0f, value)); 
}

template <class T>
inline T square(const T &x) { return x*x; };

template <class T>
inline int sign(const T &x) { return (x >= 0 ? 1 : -1); };

inline float clamp ( float value , float min , float max ) {
    if (value < min)
        return min;
    if (value > max)
        return max;

    return value;
}

inline int roundTo(int num, int factor) {
    return num + factor - 1 - (num + factor - 1) % factor;
}

inline float lerp(float a, float b, float f) {
    return a + f * (b - a);
}  

inline float remap(const float& _value, const float& _inputMin, const float& _inputMax, const float& _outputMin, const float& _outputMax, bool _clamp) {
    if (fabs(_inputMin - _inputMax) < std::numeric_limits<float>::epsilon()) { return _outputMin; } else {
        float outVal = ((_value - _inputMin) / (_inputMax - _inputMin) * (_outputMax - _outputMin) + _outputMin);

        if (_clamp) {
            if (_outputMax < _outputMin) {
                if (outVal < _outputMax) {
                    outVal = _outputMax;
                } else if (outVal > _outputMin) {
                    outVal = _outputMin;
                }
            } else {
                if (outVal > _outputMax) {
                    outVal = _outputMax;
                } else if (outVal < _outputMin) {
                    outVal = _outputMin;
                }
            }
        }
        return outVal;
    }
}

inline glm::vec2 remap(const glm::vec2& _value, const glm::vec2& _inputMin, const glm::vec2& _inputMax, const glm::vec2& _outputMin, const glm::vec2& _outputMax, bool _clamp) {
    return glm::vec2(   remap(_value.x, _inputMin.x, _inputMax.x, _outputMin.x, _outputMax.x, _clamp),
                        remap(_value.y, _inputMin.y, _inputMax.y, _outputMin.y, _outputMax.y, _clamp) );
}

inline glm::vec3 remap(const glm::vec3& _value, const glm::vec3& _inputMin, const glm::vec3& _inputMax, const glm::vec3& _outputMin, const glm::vec3& _outputMax, bool _clamp) {
    return glm::vec3(   remap(_value.x, _inputMin.x, _inputMax.x, _outputMin.x, _outputMax.x, _clamp),
                        remap(_value.y, _inputMin.y, _inputMax.y, _outputMin.y, _outputMax.y, _clamp),
                        remap(_value.z, _inputMin.z, _inputMax.z, _outputMin.z, _outputMax.z, _clamp) );
}

inline glm::vec4 remap(const glm::vec4& _value, const glm::vec4& _inputMin, const glm::vec4& _inputMax, const glm::vec4& _outputMin, const glm::vec4& _outputMax, bool _clamp) {
    return glm::vec4(   remap(_value.x, _inputMin.x, _inputMax.x, _outputMin.x, _outputMax.x, _clamp),
                        remap(_value.y, _inputMin.y, _inputMax.y, _outputMin.y, _outputMax.y, _clamp),
                        remap(_value.z, _inputMin.z, _inputMax.z, _outputMin.z, _outputMax.z, _clamp),
                        remap(_value.w, _inputMin.w, _inputMax.w, _outputMin.w, _outputMax.w, _clamp) );
}

inline glm::mat2 inverse(const glm::mat2& m) {
    return glm::mat2(   m[1][1],-m[0][1], -m[1][0], m[0][0]) / (m[0][0]*m[1][1] - m[0][1]*m[1][0]);
}

inline glm::mat3 inverse(const glm::mat3& m) {
    float   b01 =  m[2][2] * m[1][1] - m[1][2] * m[2][1],
            b11 = -m[2][2] * m[1][0] + m[1][2] * m[2][0],
            b21 =  m[2][1] * m[1][0] - m[1][1] * m[2][0];

    float det = m[0][0] * b01 + m[0][1] * b11 + m[0][2] * b21;

    return glm::mat3(   b01, (-m[2][2] * m[0][1] + m[0][2] * m[2][1]), (m[1][2] * m[0][1] - m[0][2] * m[1][1]),
                        b11, (m[2][2] * m[0][0] - m[0][2] * m[2][0]), (-m[1][2] * m[0][0] + m[0][2] * m[1][0]),
                        b21, (-m[2][1] * m[0][0] + m[0][1] * m[2][0]), (m[1][1] * m[0][0] - m[0][1] * m[1][0])) / det;
}

inline glm::mat4 inverse(const glm::mat4& m) {
    float   b00 = m[0][0] * m[1][1] - m[0][1] * m[1][0],
            b01 = m[0][0] * m[1][2] - m[0][2] * m[1][0],
            b02 = m[0][0] * m[1][3] - m[0][3] * m[1][0],
            b03 = m[0][1] * m[1][2] - m[0][2] * m[1][1],
            b04 = m[0][1] * m[1][3] - m[0][3] * m[1][1],
            b05 = m[0][2] * m[1][3] - m[0][3] * m[1][2],
            b06 = m[2][0] * m[3][1] - m[2][1] * m[3][0],
            b07 = m[2][0] * m[3][2] - m[2][2] * m[3][0],
            b08 = m[2][0] * m[3][3] - m[2][3] * m[3][0],
            b09 = m[2][1] * m[3][2] - m[2][2] * m[3][1],
            b10 = m[2][1] * m[3][3] - m[2][3] * m[3][1],
            b11 = m[2][2] * m[3][3] - m[2][3] * m[3][2],

            det = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;

  return glm::mat4( m[1][1] * b11 - m[1][2] * b10 + m[1][3] * b09,
                    m[0][2] * b10 - m[0][1] * b11 - m[0][3] * b09,
                    m[3][1] * b05 - m[3][2] * b04 + m[3][3] * b03,
                    m[2][2] * b04 - m[2][1] * b05 - m[2][3] * b03,
                    m[1][2] * b08 - m[1][0] * b11 - m[1][3] * b07,
                    m[0][0] * b11 - m[0][2] * b08 + m[0][3] * b07,
                    m[3][2] * b02 - m[3][0] * b05 - m[3][3] * b01,
                    m[2][0] * b05 - m[2][2] * b02 + m[2][3] * b01,
                    m[1][0] * b10 - m[1][1] * b08 + m[1][3] * b06,
                    m[0][1] * b08 - m[0][0] * b10 - m[0][3] * b06,
                    m[3][0] * b04 - m[3][1] * b02 + m[3][3] * b00,
                    m[2][1] * b02 - m[2][0] * b04 - m[2][3] * b00,
                    m[1][1] * b07 - m[1][0] * b09 - m[1][2] * b06,
                    m[0][0] * b09 - m[0][1] * b07 + m[0][2] * b06,
                    m[3][1] * b01 - m[3][0] * b03 - m[3][2] * b00,
                    m[2][0] * b03 - m[2][1] * b01 + m[2][2] * b00) / det;
}

}