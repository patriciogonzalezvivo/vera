#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <cstring>
#include <string>
#include <cmath>

namespace vera {

/// Image channel formats
enum Channels {
    LUMINANCE = 1,          ///< Grayscale/luminance only
    LUMINANCE_ALPHA = 2,    ///< Grayscale + alpha
    RGB = 3,                ///< Red, green, blue
    RGB_ALPHA = 4           ///< Red, green, blue, alpha
};

// =============================================================================
// PIXEL SAVING (implementations in gltf.cpp due to stb_image dependencies)
// =============================================================================

/// Save 8-bit pixel data to image file
/// @param _path Output file path (format determined by extension)
/// @param _pixels Pixel data buffer
/// @param _width Image width in pixels
/// @param _height Image height in pixels
/// @return True if successfully saved
bool            savePixels(const std::string& _path, unsigned char* _pixels, int _width, int _height);

/// Save 16-bit pixel data to image file
/// @param _path Output file path
/// @param _pixels 16-bit pixel data buffer
/// @param _width Image width in pixels
/// @param _height Image height in pixels
/// @return True if successfully saved
bool            savePixels16(const std::string& _path, unsigned short* _pixels, int _width, int _height);

/// Save float pixel data to image file (typically HDR formats)
/// @param _path Output file path
/// @param _pixels Float pixel data buffer
/// @param _width Image width in pixels
/// @param _height Image height in pixels
/// @return True if successfully saved
bool            savePixelsFloat(const std::string& _path, float* _pixels, int _width, int _height);

// =============================================================================
// PIXEL LOADING
// =============================================================================

/// Load pixel data from memory buffer
/// @param _data Pointer to encoded image data
/// @param len Length of data buffer in bytes
/// @param _width Output width in pixels
/// @param _height Output height in pixels
/// @param _channels Desired channel format
/// @param _vFlip Flip image vertically if true
/// @return Allocated pixel buffer (caller must free with freePixels)
unsigned char*  loadPixels(unsigned char const *_data, int len, int *_width, int *_height, Channels _channels, bool _vFlip);

/// Load 8-bit pixel data from image file
/// @param _path Image file path
/// @param _width Output width in pixels
/// @param _height Output height in pixels
/// @param _channels Desired channel format (default: RGB)
/// @param _vFlip Flip image vertically if true (default: true)
/// @return Allocated pixel buffer (caller must free with freePixels)
unsigned char*  loadPixels(const std::string& _path, int *_width, int *_height, Channels _channels = RGB, bool _vFlip = true);

/// Load 16-bit pixel data from image file
/// @param _path Image file path
/// @param _width Output width in pixels
/// @param _height Output height in pixels
/// @param _channels Desired channel format (default: RGB)
/// @param _vFlip Flip image vertically if true (default: true)
/// @return Allocated 16-bit pixel buffer (caller must free with freePixels)
uint16_t *      loadPixels16(const std::string& _path, int *_width, int *_height, Channels _channels = RGB, bool _vFlip = true);

/// Load depth map pixel data from image file
/// @param _path Image file path
/// @param _width Output width in pixels
/// @param _height Output height in pixels
/// @param _vFlip Flip image vertically if true (default: true)
/// @return Allocated pixel buffer containing depth data
unsigned char*  loadPixelsDepth(const std::string& _path, int *_width, int *_height, bool _vFlip = true);

/// Load pixel data from Base64-encoded string
/// @param _base64 Base64-encoded image data
/// @param _width Output width in pixels
/// @param _height Output height in pixels
/// @param _channels Desired channel format (default: RGB_ALPHA)
/// @param _vFlip Flip image vertically if true (default: true)
/// @return Allocated pixel buffer (caller must free with freePixels)
unsigned char*  loadPixelsBase64(const std::string& _base64, int *_width, int *_height, Channels _channels = RGB_ALPHA, bool _vFlip = true);

/// Load floating-point pixel data from image file
/// @param _path Image file path (typically HDR formats)
/// @param _width Output width in pixels
/// @param _height Output height in pixels
/// @param _channels Output number of channels
/// @param _vFlip Flip image vertically if true (default: true)
/// @return Allocated float pixel buffer (caller must free with freePixels)
float*          loadPixelsFloat(const std::string& _path, int *_width, int *_height, int* _channels, bool _vFlip = true);

/// Free pixel buffer allocated by load functions
/// @param pixels Pixel buffer to free
void            freePixels(void *pixels);

// =============================================================================
// PIXEL MANIPULATION
// =============================================================================

/// Rescale pixel data to new dimensions using nearest-neighbor sampling
/// @tparam T Pixel data type (unsigned char, unsigned short, float, etc.)
/// @param _src Source pixel buffer
/// @param _srcWidth Source width in pixels
/// @param _srcHeight Source height in pixels
/// @param _srcChannels Number of channels per pixel
/// @param _dstWidth Destination width in pixels
/// @param _dstHeight Destination height in pixels
/// @param _dst Destination buffer (must be pre-allocated)
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

/// Flip pixel data vertically (in-place)
/// @tparam T Pixel data type
/// @param _pixels Pixel buffer to flip
/// @param _width Image width in pixels
/// @param _height Image height in pixels
/// @param _bytes_per_pixel Number of bytes per pixel (width * channels * sizeof(T))
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

// =============================================================================
// COLOR SPACE CONVERSION (Linear RGB ↔ sRGB)
// =============================================================================

/// Convert linear RGB float value to sRGB 8-bit
/// @param _value Linear RGB value in [0, 1]
/// @return sRGB value in [0, 255]
inline unsigned char linearToSrgb(float _value) {
    if (_value <= 0.0031308f) {
        return static_cast<unsigned char>(255 * _value * 12.92f);
    } else {
        return static_cast<unsigned char>(255 * (1.055f * pow(_value, 1.0f / 2.4f) - 0.055f));
    }
}

/// Convert linear RGB 8-bit value to sRGB 8-bit
/// @param _value Linear RGB value in [0, 255]
/// @return sRGB value in [0, 255]
inline unsigned char linearToSrgb(unsigned char _value) {
    return linearToSrgb(static_cast<float>(_value) / 255.0f);
}

/// Convert linear RGB 16-bit value to sRGB 8-bit
/// @param _value Linear RGB value in [0, 65535]
/// @return sRGB value in [0, 255]
inline unsigned char linearToSrgb(unsigned short _value) {
    return linearToSrgb(static_cast<float>(_value) / 65535.0f); 
}

/// Convert entire pixel buffer from linear RGB to sRGB (in-place)
/// @tparam T Pixel data type
/// @param _pixels Pixel buffer to convert
/// @param _width Image width in pixels
/// @param _height Image height in pixels
/// @param _bytes_per_pixel Number of bytes per pixel
template<typename T>
void linearToSrgb(T *_pixels, int _width, int _height, int _bytes_per_pixel) {
    for (int i = 0; i < _width * _height * _bytes_per_pixel; i++) {
        _pixels[i] = linearToSrgb(_pixels[i]);
    }
}

/// Convert sRGB 8-bit value to linear RGB float
/// @param _value sRGB value in [0, 255]
/// @return Linear RGB value in [0, 1]
inline float srgbToLinear(unsigned char _value) {
    if (_value <= 0.04045f * 255.0f) {
        return static_cast<float>(_value) / 255.0f / 12.92f;
    } else {
        return pow((static_cast<float>(_value) / 255.0f + 0.055f) / 1.055f, 2.4f);
    }
}

/// Convert sRGB 16-bit value to linear RGB float
/// @param _value sRGB value in [0, 65535]
/// @return Linear RGB value in [0, 1]
inline float srgbToLinear(unsigned short _value) {
    if (_value <= 0.04045f * 65535.0f) {
        return static_cast<float>(_value) / 65535.0f / 12.92f;
    } else {
        return pow((static_cast<float>(_value) / 65535.0f + 0.055f) / 1.055f, 2.4f);
    }
}

/// Convert 8-bit pixel buffer from sRGB to linear RGB (in-place)
/// @param _pixels Pixel buffer to convert
/// @param _width Image width in pixels
/// @param _height Image height in pixels
/// @param _bytes_per_pixel Number of bytes per pixel
inline void srgbToLinear(unsigned char *_pixels, int _width, int _height, int _bytes_per_pixel) {
    for (int i = 0; i < _width * _height * _bytes_per_pixel; i++) {
        _pixels[i] = static_cast<unsigned char>(srgbToLinear(_pixels[i]) * 255.0f);
    }
}

/// Convert pixel buffer from sRGB to linear RGB (in-place)
/// @tparam T Pixel data type
/// @param _pixels Pixel buffer to convert
/// @param _width Image width in pixels
/// @param _height Image height in pixels
/// @param _bytes_per_pixel Number of bytes per pixel
template<typename T>
inline void srgbToLinear(T *_pixels, int _width, int _height, int _bytes_per_pixel) {
    for (int i = 0; i < _width * _height * _bytes_per_pixel; i++) {
        _pixels[i] = static_cast<T>(srgbToLinear(static_cast<unsigned char>(_pixels[i])) * 255.0f);
    }
}


}
