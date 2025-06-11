#include "vera/gl/uniform.h"

namespace vera {

UniformData::UniformData() {
    size = 0;
}

UniformData::UniformData(int _x) {
    value[0] = _x;
    size = 1;
    bInt = true;
}
UniformData::UniformData(int _x, int _y) {
    value[0] = _x;
    value[1] = _y;
    size = 2;
    bInt = true;
}

UniformData::UniformData(int _x, int _y, int _z) {
    value[0] = _x;
    value[1] = _y;
    value[2] = _z;
    size = 3;
    bInt = true;
}

UniformData::UniformData(int _x, int _y, int _z, int _w) {
    value[0] = _x;
    value[1] = _y;
    value[2] = _z;
    value[3] = _w;
    size = 4;
    bInt = true;
}

UniformData::UniformData(const int *_array, size_t _size) {
    size = (_size > 4) ? 4 : _size;
    for (size_t i = 0; i < size; ++i)
        value[i] = _array[i];
    bInt = true;
}

UniformData::UniformData(float _x) {
    value[0] = _x;
    size = 1;
    bInt = false;
}

UniformData::UniformData(float _x, float _y) {
    value[0] = _x;
    value[1] = _y;
    size = 2;
    bInt = false;
}

UniformData::UniformData(float _x, float _y, float _z) {
    value[0] = _x;
    value[1] = _y;
    value[2] = _z;
    size = 3;
    bInt = false;
}

UniformData::UniformData(float _x, float _y, float _z, float _w) {
    value[0] = _x;
    value[1] = _y;
    value[2] = _z;
    value[3] = _w;
    size = 4;
    bInt = false;
}

UniformData::UniformData(const float *_array, size_t _size) {
    size = (_size > 4) ? 4 : _size;
    for (size_t i = 0; i < size; ++i)
        value[i] = _array[i];
    bInt = false;
}

UniformData::UniformData(const glm::vec2& _value) {
    value[0] = _value.x;
    value[1] = _value.y;
    size = 2;
    bInt = false;
}

UniformData::UniformData(const glm::vec3& _value) {
    value[0] = _value.x;
    value[1] = _value.y;
    value[2] = _value.z;
    size = 3;
    bInt = false;
}

UniformData::UniformData(const glm::vec4& _value) {
    value[0] = _value.x;
    value[1] = _value.y;
    value[2] = _value.z;
    value[3] = _value.w;
    size = 4;
    bInt = false;
}

UniformData::UniformData(const glm::mat3& _value, bool _transpose) {
    size = 9;
    bInt = false;
    bTranspose = _transpose;
    for (size_t i = 0; i < size; ++i)
        value[i] = reinterpret_cast<const float*>(&_value)[i];
}

UniformData::UniformData(const glm::mat4& _value, bool _transpose ) {
    size = 16;
    bInt = false;
    bTranspose = _transpose;
    for (size_t i = 0; i < size; ++i)
        value[i] = reinterpret_cast<const float*>(&_value)[i];
}

};
