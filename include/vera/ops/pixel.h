#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <cstring>
#include <string>
#include <cmath>

namespace vera {

enum Channels {
    LUMINANCE = 1,
    LUMINANCE_ALPHA = 2,
    RGB = 3,
    RGB_ALPHA = 4
};

// Implementation of savePixels and loadPixels is on gltf.cpp because tiny_gltf.h also use stb_image*.h
bool            savePixels(const std::string& _path, unsigned char* _pixels, int _width, int _height);
bool            savePixels16(const std::string& _path, unsigned short* _pixels, int _width, int _height);
bool            savePixelsFloat(const std::string& _path, float* _pixels, int _width, int _height);

unsigned char*  loadPixels(unsigned char const *_data, int len, int *_width, int *_height, Channels _channels, bool _vFlip);
unsigned char*  loadPixels(const std::string& _path, int *_width, int *_height, Channels _channels = RGB, bool _vFlip = true);
uint16_t *      loadPixels16(const std::string& _path, int *_width, int *_height, Channels _channels = RGB, bool _vFlip = true);
unsigned char*  loadPixelsDepth(const std::string& _path, int *_width, int *_height, bool _vFlip = true);
unsigned char*  loadPixelsBase64(const std::string& _base64, int *_width, int *_height, Channels _channels = RGB_ALPHA, bool _vFlip = true);
float*          loadPixelsFloat(const std::string& _path, int *_width, int *_height, int* _channels, bool _vFlip = true);
void            freePixels(void *pixels);

template<typename T>
void rescalePixels(const T* _src, int _srcWidth, int _srcHeight, int _srcChannels, int _dstWidth, int _dstHeight, T* _dst) {
    int x, y, i, c;
    int x_src, y_src, i_src;
    for (y = 0; y < _dstHeight; y++) {
        for (x = 0; x < _dstWidth; x++) {
            i = y * _dstWidth + x;
            y_src = ((float)(y)/(float)(_dstHeight-1)) * (_srcHeight-1);
            x_src = ((float)(x)/(float)(_dstWidth-1)) * (_srcWidth-1);
            i_src = y_src * _srcWidth + x_src;

            i *= _srcChannels;
            i_src *= _srcChannels;

            for (c = 0; c < _srcChannels; c++) {
                _dst[i + c] = _src[i_src + c];
            }
        }
    }
}

template<typename T>
void flipPixelsVertically(T *_pixels, int _width, int _height, int _bytes_per_pixel) {
    const size_t stride = _width * _bytes_per_pixel;
    T *row = (T*)malloc(stride * sizeof(T));
    T *low = _pixels;
    T *high = &_pixels[(_height - 1) * stride];
    for (; low < high; low += stride, high -= stride) {
        std::memcpy(row, low, stride * sizeof(T));
        std::memcpy(low, high, stride * sizeof(T));
        std::memcpy(high, row, stride * sizeof(T));
    }
    free(row);
}

// Convert from linear RGB to sRGB
inline unsigned char linearToSrgb(float _value) {
    if (_value <= 0.0031308f) {
        return static_cast<unsigned char>(255 * _value * 12.92f);
    } else {
        return static_cast<unsigned char>(255 * (1.055f * pow(_value, 1.0f / 2.4f) - 0.055f));
    }
}

inline unsigned char linearToSrgb(unsigned char _value) {
    return linearToSrgb(static_cast<float>(_value) / 255.0f);
}

inline unsigned char linearToSrgb(unsigned short _value) {
    return linearToSrgb(static_cast<float>(_value) / 65535.0f); 
}

template<typename T>
void linearToSrgb(T *_pixels, int _width, int _height, int _bytes_per_pixel) {
    for (int i = 0; i < _width * _height * _bytes_per_pixel; i++) {
        _pixels[i] = linearToSrgb(_pixels[i]);
    }
}

// Convert from sRGB to linear RGB
inline float srgbToLinear(unsigned char _value) {
    if (_value <= 0.04045f * 255.0f) {
        return static_cast<float>(_value) / 255.0f / 12.92f;
    } else {
        return pow((static_cast<float>(_value) / 255.0f + 0.055f) / 1.055f, 2.4f);
    }
}

inline float srgbToLinear(unsigned short _value) {
    if (_value <= 0.04045f * 65535.0f) {
        return static_cast<float>(_value) / 65535.0f / 12.92f;
    } else {
        return pow((static_cast<float>(_value) / 65535.0f + 0.055f) / 1.055f, 2.4f);
    }
}

inline void srgbToLinear(unsigned char *_pixels, int _width, int _height, int _bytes_per_pixel) {
    for (int i = 0; i < _width * _height * _bytes_per_pixel; i++) {
        _pixels[i] = static_cast<unsigned char>(srgbToLinear(_pixels[i]) * 255.0f);
    }
}

template<typename T>
inline void srgbToLinear(T *_pixels, int _width, int _height, int _bytes_per_pixel) {
    for (int i = 0; i < _width * _height * _bytes_per_pixel; i++) {
        _pixels[i] = static_cast<T>(srgbToLinear(static_cast<unsigned char>(_pixels[i])) * 255.0f);
    }
}


}
