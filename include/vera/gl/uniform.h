#pragma once

#include <string>
#include <map>
#include <array>
#include <glm/glm.hpp>

// UniformData — type-erased storage for a single GLSL uniform value.
// Values are stored as a fixed-size float array (up to 16 elements, enough
// for a mat4). bInt distinguishes float uniforms from integer uniforms;
// bTranspose is used by matrix uniforms. size records the component count
// (1–4 for scalars/vectors, 9 for mat3, 16 for mat4).
//
// UniformTextureMap entries pair a texture unit location (texLoc) with the
// GL texture id so updateUniforms() can rebind textures after a program
// switch without requiring the caller to repeat setUniformTexture().

namespace vera {

typedef std::array<float, 16> UniformValue;

struct UniformData {
    UniformData();
    UniformData(int _x);
    UniformData(int _x, int _y);
    UniformData(int _x, int _y, int _z);
    UniformData(int _x, int _y, int _z, int _w);
    UniformData(const int *_array, size_t _size);
    UniformData(float _x);
    UniformData(float _x, float _y);
    UniformData(float _x, float _y, float _z);
    UniformData(float _x, float _y, float _z, float _w);
    UniformData(const float *_array, size_t _size);
    UniformData(const glm::vec2& _value);
    UniformData(const glm::vec3& _value);
    UniformData(const glm::vec4& _value);
    UniformData(const glm::mat3& _value, bool _transpose = false);
    UniformData(const glm::mat4& _value, bool _transpose = false);

    UniformValue                        value;
    size_t                              size;
    bool                                bInt    = false;
    bool                                bTranspose = false;
};

typedef std::map<std::string, UniformData>  UniformDataMap;

enum UniformTextureType {
    TEXTURE_2D = 0,
    TEXTURE_CUBE = 1
};

struct UniformTexture {
    UniformTexture() = default;
    UniformTexture(UniformTextureType _type, size_t _id, size_t _texLoc) {
        type = _type;
        id = _id;
        texLoc = _texLoc;
    }

    UniformTextureType type;
    size_t      id;
    size_t      texLoc;
};

typedef std::map<std::string, UniformTexture>  UniformTextureMap;

};
