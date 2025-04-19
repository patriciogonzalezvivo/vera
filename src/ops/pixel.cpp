#include <iostream>

#include "vera/ops/pixel.h"
#include "vera/ops/fs.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"
#include "extract_depthmap.h"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

namespace vera {

unsigned char* loadPixels(unsigned char const *_data, int len, int *_width, int *_height, vera::Channels _channels, bool _vFlip) {
    int comp;
    unsigned char* pixels = stbi_load_from_memory(_data, len, _width, _height, &comp, (_channels == RGB)? STBI_rgb : STBI_rgb_alpha);
    return pixels;
} 

unsigned char* loadPixels(const std::string& _path, int *_width, int *_height, Channels _channels, bool _vFlip) {
    stbi_set_flip_vertically_on_load(_vFlip);
    int comp;
    unsigned char* pixels = stbi_load(_path.c_str(), _width, _height, &comp, (_channels == RGB)? STBI_rgb : STBI_rgb_alpha);
    return pixels;
}

uint16_t* loadPixels16(const std::string& _path, int *_width, int *_height, Channels _channels, bool _vFlip) {
    stbi_set_flip_vertically_on_load(_vFlip);
    int comp;
    uint16_t *pixels = stbi_load_16(_path.c_str(), _width, _height, &comp, _channels);
    return pixels;
}

float* loadPixelsFloat(const std::string& _path, int *_width, int *_height, int* _channels, bool _vFlip) {
    std::string ext = getExt(_path);

    if (ext == "hdr" || ext == "HDR") {    
        stbi_set_flip_vertically_on_load(_vFlip);
        int comp;
        *_channels = 3;
        return stbi_loadf(_path.c_str(), _width, _height, &comp, 0);
    }
    else if (ext == "exr" || ext == "EXR") {

        *_channels = 4;
        float* pixels;
        const char* err = NULL; // or nullptr in C++11
        int ret = LoadEXR(&pixels, _width, _height, _path.c_str(), &err);
        if (ret != TINYEXR_SUCCESS) {
            if (err) {
                fprintf(stderr, "ERR : %s\n", err);
                FreeEXRErrorMessage(err); // release memory of error message.
            }
            return nullptr;
        }

        if (_vFlip)
            flipPixelsVertically<float>(pixels, *_width, *_height, *_channels);

        return pixels;
    }

    return nullptr;
}

unsigned char* loadPixelsDepth(const std::string& _path, int *_width, int *_height, bool _vFlip) {
    const unsigned char *cv = NULL, *dm = NULL;
    size_t cv_size = 0, dm_size = 0;
    image_type_t dm_type = TYPE_NONE;

    //  proceed to check if it have depth data
    if (extract_depth(  _path.c_str(), 
                        &cv, &cv_size,
                        &dm, &dm_size, &dm_type) == 1) {

        if (dm_type == TYPE_JPEG) {
            unsigned char* pixels_depth = loadPixels(dm, dm_size, _width, _height, RGB, _vFlip);
            free(reinterpret_cast<void*>(const_cast<unsigned char*>(dm)));
            return pixels_depth;
        }
    }

    return nullptr;
}

unsigned char* loadPixelsBase64(const std::string& _base64, int *_width, int *_height, Channels _channels, bool _vFlip) {
    unsigned char* data = new unsigned char[_base64.size()];
    size_t len = decodeBase64(_base64, data);
    int comp;
    unsigned char* pixels = stbi_load_from_memory(data, len, _width, _height, &comp, (_channels == RGB)? STBI_rgb : STBI_rgb_alpha);
    return pixels;
}

bool savePixels(const std::string& _path, unsigned char* _pixels, int _width, int _height) {
    int saved = 0;
    int channels = 4;
    std::string ext = getExt(_path);

    // Flip the image on Y
    flipPixelsVertically<unsigned char>(_pixels, _width, _height, channels);

    if ( ext == "png") 
        saved = stbi_write_png(_path.c_str(), _width, _height, channels, _pixels, _width * channels);
    else if ( ext == "jpg")
        saved = stbi_write_jpg(_path.c_str(), _width, _height, channels, _pixels, 92);
    else if ( ext == "bmp")
        saved = stbi_write_bmp(_path.c_str(), _width, _height, channels, _pixels);
    else if ( ext == "tga")
        saved = stbi_write_tga(_path.c_str(), _width, _height, channels, _pixels);
    else if ( ext == "hdr") {
        size_t total = _width * _height * channels;
        const float m = 1.f / 255.f;
        float *float_pixels = new float[total];
        for (size_t i = 0; i < total; i++)
            float_pixels[i] = _pixels[i] * m;
        saved = stbi_write_hdr(_path.c_str(), _width, _height, channels, float_pixels);
        delete [] float_pixels;
    }

    if (0 == saved) {
        std::cout << "Can't create file " << _path << std::endl;
        return false;
    }
    return true;
}

bool savePixels16(const std::string& _path, unsigned short* _pixels, int _width, int _height) {
    int saved = 0;
    int channels = 4;
    std::string ext = getExt(_path);

    // Flip the image on Y
    flipPixelsVertically<unsigned short>(_pixels, _width, _height, channels);

    if ( ext == "png") 
        saved = stbi_write_png(_path.c_str(), _width, _height, channels, _pixels, _width * channels);
    else if ( ext == "jpg")
        saved = stbi_write_jpg(_path.c_str(), _width, _height, channels, _pixels, 92);
    else if ( ext == "bmp")
        saved = stbi_write_bmp(_path.c_str(), _width, _height, channels, _pixels);
    else if ( ext == "tga")
        saved = stbi_write_tga(_path.c_str(), _width, _height, channels, _pixels);
    else if ( ext == "hdr") {
        size_t total = _width * _height * channels;
        const float m = 1.f / 65535.f;
        float *float_pixels = new float[total];
        for (size_t i = 0; i < total; i++)
            float_pixels[i] = _pixels[i] * m;
        saved = stbi_write_hdr(_path.c_str(), _width, _height, channels, float_pixels);
        delete [] float_pixels;
    }

    if (0 == saved) {
        std::cout << "Can't create file " << _path << std::endl;
        return false;
    }
    return true;
}

bool savePixelsFloat(const std::string& _path, float* _pixels, int _width, int _height) {
    int channels = 4;

    // Flip the image on Y
    flipPixelsVertically<float>(_pixels, _width, _height, channels);
    
    std::string ext = getExt(_path);
    if (ext == "hdr" || ext == "HDR") {    
        if (0 == stbi_write_hdr(_path.c_str(), _width, _height, channels, _pixels)) {
            std::cout << "Can't create file " << _path << std::endl;
            return false;
        }
    }
    else if (ext == "exr" || ext == "EXR") { 
        // TODO
    }

    return true;
}

void freePixels(void *pixels) {
    stbi_image_free(pixels);
}

}