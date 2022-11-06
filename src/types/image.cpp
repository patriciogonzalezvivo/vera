#include <iostream>
#include <stdio.h>
#include <cstring>

#include "vera/types/image.h"
#include "vera/ops/fs.h"
#include "vera/ops/math.h"
#include "vera/ops/pixel.h"

namespace vera {

Image::Image(): m_path(""), m_width(0), m_height(0), m_channels(0) {
}

Image::Image(const Image& _mother): m_path("") {
    m_width = _mother.m_width;
    m_height = _mother.m_height;
    m_channels = _mother.m_channels;

    int total = m_width * m_height * m_channels;
    if (_mother.isAllocated()) {
        m_data.resize(total);
        std::memcpy(&m_data[0], &_mother.m_data[0], total * sizeof(float));
    }
}

Image::Image(int _width, int _height, int _channels): m_path("") {
    allocate(_width, _height, _channels);
}

// bool Image::loadData(const uint8_t* _array3D, int _height, int _width, int _channels) {
Image::Image(const uint8_t* _array3D, int _height, int _width, int _channels): m_path("") {
    set(_array3D, _height, _width, _channels);
}

Image::~Image() {
}

bool Image::load(const std::string& _path, bool _flip) {
     if (!urlExists(_path))
        return false;

    m_path = _path;
    
    std::string ext = getExt(_path);
    bool loaded = false;

    // BMP non-1bpp, non-RLE
    // GIF (*comp always reports as 4-channel)
    // JPEG baseline & progressive (12 bpc/arithmetic not supported, same as stock IJG lib)
    if (ext == "bmp"    || ext == "BMP" ||
        ext == "gif"    || ext == "GIF" ||
        ext == "jpg"    || ext == "JPG" ||
        ext == "jpeg"   || ext == "JPEG" ) {

        unsigned char* pixels = loadPixels(_path, &m_width, &m_height, RGB_ALPHA, _flip);
        m_channels = 4;
        int total = m_width * m_height * m_channels;
        if (total != m_data.size())
            m_data.resize(total);
        const float m = 1.f / 255.f;
        for (int i = 0; i < total; i++)
            setValue(i, float(pixels[i]) * m);
        freePixels(pixels);
        return true;
    }

    // PNG 1/2/4/8/16-bit-per-channel
    // TGA (not sure what subset, if a subset)
    // PSD (composited view only, no extra channels, 8/16 bit-per-channel)
    else if (   ext == "png" || ext == "PNG" ||
                ext == "psd" || ext == "PSD" ||
                ext == "tga" || ext == "TGA") {
#if defined(PLATFORM_RPI) || defined(__EMSCRIPTEN__)
        // If we are in a Raspberry Pi don't take the risk of loading a 16bit image
        unsigned char* pixels = loadPixels(_path, &m_width, &m_height, RGB_ALPHA, _flip);
        m_channels = 4;
        int total = m_width * m_height * m_channels;
        if (total != m_data.size())
            m_data.resize(total);
        const float m = 1.f / 255.f;
        for (int i = 0; i < total; i++)
            setValue(i, float(pixels[i]) * m);
        freePixels(pixels);
        return true;
#else
        uint16_t* pixels = loadPixels16(_path, &m_width, &m_height, RGB_ALPHA, _flip);
        m_channels = 4;
        int total = m_width * m_height * m_channels;
        if (total != m_data.size())
            m_data.resize(total);
        const float m = 1.f / 65535.f;
        for (int i = 0; i < total; i++)
            setValue(i, float(pixels[i]) * m);
        freePixels(pixels);
        return true;
#endif
    }

    // HDR (radiance rgbE format)
    else if (ext == "hdr" || ext == "HDR") {
        float* pixels = loadPixelsHDR(_path, &m_width, &m_height, _flip);
        m_channels = 3;
        int total = m_width * m_height * m_channels;
        if (total != m_data.size())
            m_data.resize(total);
        std::memcpy(&m_data[0], pixels, total * sizeof(float));
        freePixels(pixels);
        return true;
    }

    return false;
}

bool Image::save(const std::string& _filepath, bool _vFlip) {
    size_t total = m_width * m_height;
    unsigned short *pixels = new unsigned short[total * 4];
    for (size_t i = 0; i < total; i++) {
        glm::vec4 c = getColor(i);
        pixels[i * 4 + 0] = c.r * 65535;
        pixels[i * 4 + 1] = c.g * 65535;
        pixels[i * 4 + 2] = c.b * 65535;
        pixels[i * 4 + 3] = c.a * 65535;
    }

    return vera::savePixels16(_filepath, pixels, m_width, m_height);
    freePixels(pixels);
}

bool Image::allocate(size_t _width, size_t _height, size_t _channels) {
    m_width = _width;
    m_height = _height;
    m_channels = _channels;
    // data = new float[_width * _height * _channels];
    int total = _width * _height * _channels;

    if (total != m_data.size())
        m_data.resize(total);

    return true;
}

void Image::set(const uint8_t* _array3D, int _height, int _width, int _channels) {
    allocate(_width, _height, _channels);
    int total = m_width * m_height * m_channels;
    m_data.resize(total);
    for (int i = 0; i < total; i++)
        m_data[i] = _array3D[i]/255.0f;
}

void Image::setValue(size_t _index, float _data) {
    if (m_data.size() == 0) {
        std::cout << "Data have been not pre allocated" << std::endl;
        return;
    }
    m_data[_index] = _data; 
}

void Image::setValue(size_t _index, const float* _array1D, int _n) {
    if (m_data.size() == 0) {
        std::cout << "Data have been not pre allocated" << std::endl;
        return;
    }

    std::memcpy(&m_data[_index], _array1D, _n * sizeof(float));
}

void Image::setColors(const float* _array2D, int _m, int _n) {
    std::memcpy(&m_data[0], _array2D, _m * _n * sizeof(float));
}

// To numpy https://numpy.org/devdocs/reference/swig.interface-file.html
void Image::get(uint8_t **_array3D, int *_height, int *_width, int *_channels) {
    int total = m_width * m_height * m_channels;
    uint8_t * pixels = new uint8_t[total];
    for (int i = 0; i < total; i++)
        pixels[i] = static_cast<uint8_t>(256 * clamp(m_data[i], 0.0, 0.999));

    *_array3D = pixels;
    *_height = m_height;
    *_width = m_width;
    *_channels = m_channels;
}

float Image::getValue(size_t _index) const {
    if (m_data.size() == 0)
        return 0.0f;

    return m_data[_index];
}

glm::vec4 Image::getColor(size_t _index) const {
    glm::vec4 rta = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    if (m_data.size() == 0)
        return rta;

    for (size_t i = 0; i < m_channels; i++)
        rta[i] = m_data[_index + i];

    return rta;
}

Image Image::operator+ (float _value) const {
    Image out = Image(*this);
    for (size_t i = 0; i < out.m_data.size(); i++)
        out.m_data[i] = m_data[i] + _value;
    return out;
}

Image Image::operator- (float _value) const {
    Image out = Image(*this);
    for (size_t i = 0; i < out.m_data.size(); i++)
        out.m_data[i] = m_data[i] - _value;
    return out;
}

Image Image::operator* (float _value) const {
    Image out = Image(*this);
    for (size_t i = 0; i < out.m_data.size(); i++)
        out.m_data[i] = m_data[i] * _value;
    return out;
}

Image Image::operator/ (float _value) const {
    Image out = Image(*this);
    for (size_t i = 0; i < out.m_data.size(); i++)
        out.m_data[i] = m_data[i] / _value;
    return out;
}

Image& Image::operator+= (float _value) {
    for (size_t i = 0; i < m_data.size(); i++)
        m_data[i] = m_data[i] + _value;
    return *this;
}

Image& Image::operator-= (float _value) {
    for (size_t i = 0; i < m_data.size(); i++)
        m_data[i] = m_data[i] - _value;
    return *this;
}

Image& Image::operator*= (float _value) {
    for (size_t i = 0; i < m_data.size(); i++)
        m_data[i] = m_data[i] * _value;
    return *this;
}

Image& Image::operator/= (float _value) {
    for (size_t i = 0; i < m_data.size(); i++)
        m_data[i] = m_data[i] / _value;
    return *this;
}

}