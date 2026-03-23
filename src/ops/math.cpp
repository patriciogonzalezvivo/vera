#include "vera/ops/math.h"
#include <cstdlib>
#include <cmath>

namespace vera {

float angleBetween(float thetav, float phiv, float theta, float phi) {
    float cosGamma = sinf(thetav) * sinf(theta) * cosf(phi - phiv) + cosf(thetav) * cosf(theta);
    return acosf(cosGamma);
}

float saturate(float value) { 
    return std::fmax (0.0f, std::fmin (1.0f, value)); 
}

float clamp ( float value , float min , float max ) {
    if (value < min)
        return min;
    if (value > max)
        return max;

    return value;
}

int roundTo(int num, int factor) {
    return num + factor - 1 - (num + factor - 1) % factor;
}

float lerp(float a, float b, float f) {
    return a + f * (b - a);
}  

float remap(const float& _value, const float& _inputMin, const float& _inputMax, const float& _outputMin, const float& _outputMax, bool _clamp) {
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

glm::vec2 remap(const glm::vec2& _value, const glm::vec2& _inputMin, const glm::vec2& _inputMax, const glm::vec2& _outputMin, const glm::vec2& _outputMax, bool _clamp) {
    return glm::vec2(   remap(_value.x, _inputMin.x, _inputMax.x, _outputMin.x, _outputMax.x, _clamp),
                        remap(_value.y, _inputMin.y, _inputMax.y, _outputMin.y, _outputMax.y, _clamp) );
}

glm::vec3 remap(const glm::vec3& _value, const glm::vec3& _inputMin, const glm::vec3& _inputMax, const glm::vec3& _outputMin, const glm::vec3& _outputMax, bool _clamp) {
    return glm::vec3(   remap(_value.x, _inputMin.x, _inputMax.x, _outputMin.x, _outputMax.x, _clamp),
                        remap(_value.y, _inputMin.y, _inputMax.y, _outputMin.y, _outputMax.y, _clamp),
                        remap(_value.z, _inputMin.z, _inputMax.z, _outputMin.z, _outputMax.z, _clamp) );
}

glm::vec4 remap(const glm::vec4& _value, const glm::vec4& _inputMin, const glm::vec4& _inputMax, const glm::vec4& _outputMin, const glm::vec4& _outputMax, bool _clamp) {
    return glm::vec4(   remap(_value.x, _inputMin.x, _inputMax.x, _outputMin.x, _outputMax.x, _clamp),
                        remap(_value.y, _inputMin.y, _inputMax.y, _outputMin.y, _outputMax.y, _clamp),
                        remap(_value.z, _inputMin.z, _inputMax.z, _outputMin.z, _outputMax.z, _clamp),
                        remap(_value.w, _inputMin.w, _inputMax.w, _outputMin.w, _outputMax.w, _clamp) );
}

glm::mat2 inverseMatrix(const glm::mat2& m) {
    return glm::mat2(   m[1][1],-m[0][1], -m[1][0], m[0][0]) / (m[0][0]*m[1][1] - m[0][1]*m[1][0]);
}

glm::mat3 inverseMatrix(const glm::mat3& m) {
    float   b01 =  m[2][2] * m[1][1] - m[1][2] * m[2][1],
            b11 = -m[2][2] * m[1][0] + m[1][2] * m[2][0],
            b21 =  m[2][1] * m[1][0] - m[1][1] * m[2][0];

    float det = m[0][0] * b01 + m[0][1] * b11 + m[0][2] * b21;

    return glm::mat3(   b01, (-m[2][2] * m[0][1] + m[0][2] * m[2][1]), (m[1][2] * m[0][1] - m[0][2] * m[1][1]),
                        b11, (m[2][2] * m[0][0] - m[0][2] * m[2][0]), (-m[1][2] * m[0][0] + m[0][2] * m[1][0]),
                        b21, (-m[2][1] * m[0][0] + m[0][1] * m[2][0]), (m[1][1] * m[0][0] - m[0][1] * m[1][0])) / det;
}

glm::mat4 inverseMatrix(const glm::mat4& m) {
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

// =============================================================================
// RANDOM NUMBER GENERATION
// =============================================================================

void randomSeed(int seed) {
    srand(seed);
}

float random() {
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

float random(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / (max - min));
}

int random(int min, int max) {
    return min + rand() % (max - min + 1);
}

// =============================================================================
// GAUSSIAN RANDOM DISTRIBUTION
// =============================================================================

namespace {
    bool g_hasSpareGaussian = false;
    float g_spareGaussian = 0.0f;
}

float randomGaussian(float mean, float sd) {
    float val;
    
    if (g_hasSpareGaussian) {
        val = g_spareGaussian;
        g_hasSpareGaussian = false;
    }
    else {
        // Box-Muller transform to generate normally distributed random numbers
        float u, v, s;
        do {
            u = random() * 2.0f - 1.0f;
            v = random() * 2.0f - 1.0f;
            s = u * u + v * v;
        } while (s >= 1.0f || s == 0.0f);
        
        s = sqrtf(-2.0f * logf(s) / s);
        val = u * s;
        g_spareGaussian = v * s;
        g_hasSpareGaussian = true;
    }
    
    return mean + val * sd;
}

// =============================================================================
// PERLIN NOISE IMPLEMENTATION
// =============================================================================

namespace {
    // Perlin noise state
    int g_perlin_permutation[512];
    bool g_perlin_initialized = false;
    int g_perlin_octaves = 4;
    float g_perlin_amp_falloff = 0.5f;

    // Fade function for smooth interpolation (6t^5 - 15t^4 + 10t^3)
    inline float fade(float t) {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    // Linear interpolation
    inline float perlin_lerp(float t, float a, float b) {
        return a + t * (b - a);
    }

    // 1D gradient function
    inline float grad1(int hash, float x) {
        int h = hash & 15;
        float grad = 1.0f + (h & 7);
        if (h & 8) grad = -grad;
        return grad * x;
    }

    // 2D gradient function
    inline float grad2(int hash, float x, float y) {
        int h = hash & 7;
        float u = h < 4 ? x : y;
        float v = h < 4 ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
    }

    // 3D gradient function
    inline float grad3(int hash, float x, float y, float z) {
        int h = hash & 15;
        float u = h < 8 ? x : y;
        float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
        return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
    }

    // Base 1D Perlin noise
    float perlin1(float x) {
        int X = (int)floor(x) & 255;
        x -= floor(x);
        float u = fade(x);

        int A = g_perlin_permutation[X];
        int B = g_perlin_permutation[X + 1];

        return perlin_lerp(u, grad1(g_perlin_permutation[A], x),
                              grad1(g_perlin_permutation[B], x - 1));
    }

    // Base 2D Perlin noise
    float perlin2(float x, float y) {
        int X = (int)floor(x) & 255;
        int Y = (int)floor(y) & 255;

        x -= floor(x);
        y -= floor(y);

        float u = fade(x);
        float v = fade(y);

        int A = g_perlin_permutation[X] + Y;
        int B = g_perlin_permutation[X + 1] + Y;

        return perlin_lerp(v, 
            perlin_lerp(u, grad2(g_perlin_permutation[A], x, y),
                           grad2(g_perlin_permutation[B], x - 1, y)),
            perlin_lerp(u, grad2(g_perlin_permutation[A + 1], x, y - 1),
                           grad2(g_perlin_permutation[B + 1], x - 1, y - 1)));
    }

    // Base 3D Perlin noise
    float perlin3(float x, float y, float z) {
        int X = (int)floor(x) & 255;
        int Y = (int)floor(y) & 255;
        int Z = (int)floor(z) & 255;

        x -= floor(x);
        y -= floor(y);
        z -= floor(z);

        float u = fade(x);
        float v = fade(y);
        float w = fade(z);

        int A = g_perlin_permutation[X] + Y;
        int AA = g_perlin_permutation[A] + Z;
        int AB = g_perlin_permutation[A + 1] + Z;
        int B = g_perlin_permutation[X + 1] + Y;
        int BA = g_perlin_permutation[B] + Z;
        int BB = g_perlin_permutation[B + 1] + Z;

        return perlin_lerp(w,
            perlin_lerp(v,
                perlin_lerp(u, grad3(g_perlin_permutation[AA], x, y, z),
                               grad3(g_perlin_permutation[BA], x - 1, y, z)),
                perlin_lerp(u, grad3(g_perlin_permutation[AB], x, y - 1, z),
                               grad3(g_perlin_permutation[BB], x - 1, y - 1, z))),
            perlin_lerp(v,
                perlin_lerp(u, grad3(g_perlin_permutation[AA + 1], x, y, z - 1),
                               grad3(g_perlin_permutation[BA + 1], x - 1, y, z - 1)),
                perlin_lerp(u, grad3(g_perlin_permutation[AB + 1], x, y - 1, z - 1),
                               grad3(g_perlin_permutation[BB + 1], x - 1, y - 1, z - 1))));
    }

    // Initialize permutation table with given seed
    void initPerlin(unsigned int seed) {
        srand(seed);
        int p[256];
        for (int i = 0; i < 256; i++) {
            p[i] = i;
        }
        // Fisher-Yates shuffle
        for (int i = 255; i > 0; i--) {
            int j = rand() % (i + 1);
            int tmp = p[i];
            p[i] = p[j];
            p[j] = tmp;
        }
        // Duplicate permutation table to avoid overflow checks
        for (int i = 0; i < 256; i++) {
            g_perlin_permutation[i] = p[i];
            g_perlin_permutation[256 + i] = p[i];
        }
        g_perlin_initialized = true;
    }
}

// =============================================================================
// NOISE PUBLIC API (P5.js compatible)
// =============================================================================

void noiseSeed(unsigned int seed) {
    initPerlin(seed);
}

void noiseDetail(int lod, float falloff) {
    if (lod > 0) g_perlin_octaves = lod;
    if (falloff > 0.0f) g_perlin_amp_falloff = falloff;
}

float noise(float x) {
    if (!g_perlin_initialized) {
        initPerlin(0);
    }

    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;

    // Multi-octave noise (fractal Brownian motion)
    for (int i = 0; i < g_perlin_octaves; i++) {
        total += perlin1(x * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= g_perlin_amp_falloff;
        frequency *= 2.0f;
    }

    return (total / maxValue) * 0.5f + 0.5f; // Normalize to [0, 1]
}

float noise(float x, float y) {
    if (!g_perlin_initialized) {
        initPerlin(0);
    }

    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;

    // Multi-octave noise (fractal Brownian motion)
    for (int i = 0; i < g_perlin_octaves; i++) {
        total += perlin2(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= g_perlin_amp_falloff;
        frequency *= 2.0f;
    }

    return (total / maxValue) * 0.5f + 0.5f; // Normalize to [0, 1]
}

float noise(float x, float y, float z) {
    if (!g_perlin_initialized) {
        initPerlin(0);
    }

    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;

    // Multi-octave noise (fractal Brownian motion)
    for (int i = 0; i < g_perlin_octaves; i++) {
        total += perlin3(x * frequency, y * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= g_perlin_amp_falloff;
        frequency *= 2.0f;
    }

    return (total / maxValue) * 0.5f + 0.5f; // Normalize to [0, 1]
}

// GLM vector convenience overloads
float noise(const glm::vec2& v) {
    return noise(v.x, v.y);
}

float noise(const glm::vec3& v) {
    return noise(v.x, v.y, v.z);
}

}
