#include <iostream>
#include <cstring>

#include "vera/gl/textureCube.h"
#include "vera/gl/cubemapFace.h"

#include "vera/ops/fs.h"
#include "vera/ops/env.h"
#include "vera/ops/math.h"
#include "vera/ops/pixel.h"

namespace vera {

TextureCube::TextureCube() 
    : SH {  glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0),
            glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0),
            glm::vec3(0.0), glm::vec3(0.0), glm::vec3(0.0) } {
}

TextureCube::~TextureCube() {
}

bool TextureCube::load(const std::string &_path, bool _vFlip) {
    std::string ext = getExt(_path);

    if (m_id == 0)
        glGenTextures(1, &m_id);
        
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);

    int sh_samples = 0;
    if (ext == "png"    || ext == "PNG" ||
        ext == "jpg"    || ext == "JPG" ||
        ext == "jpeg"   || ext == "JPEG" ) {

        unsigned char* data = loadPixels(_path, &m_width, &m_height, RGB, false);

        // LOAD FACES
        CubemapFace<unsigned char> **faces = new CubemapFace<unsigned char>*[6];

        if (m_height > m_width) {

            if (m_width/6 == m_height) {
                // Vertical Row
                splitFacesFromVerticalRow<unsigned char>(data, m_width, m_height, 3, faces);
            }
            else {
                // Vertical Cross
                splitFacesFromVerticalCross<unsigned char>(data, m_width, m_height, 3, faces);

                // adjust NEG_Z face
                if (_vFlip) {
                    faces[5]->flipHorizontal();
                    faces[5]->flipVertical();
                }
            }
            
        }
        else {
            if (m_width/2 == m_height) {
                // Equilateral
                splitFacesFromEquirectangular<unsigned char>(data, m_width, m_height, 3, faces);
            }
            else if (m_width/6 == m_height) {
                // Horizontal Row
                splitFacesFromHorizontalRow<unsigned char>(data, m_width, m_height, 3, faces);
            }
            else {
                // Horizontal Cross
                splitFacesFromHorizontalCross<unsigned char>(data, m_width, m_height, 3, faces);
            }
        }
        
        for (int i = 0; i < 6; i++) {
            faces[i]->upload();
            sh_samples += faces[i]->calculateSH(SH);
        }

        delete[] data;
        for(int i = 0; i < 6; ++i) {
            delete[] faces[i]->data;
            delete faces[i];
        }
        delete[] faces;

    }

    else if (ext == "hdr" || ext == "HDR") {
        int channels = 3;
        float* data = loadPixelsFloat(_path, &m_width, &m_height, &channels, false);

        // LOAD FACES
        CubemapFace<float> **faces = new CubemapFace<float>*[6];

        if (m_height > m_width) {
            // Vertical Row
            if (m_width/6 == m_height)
                splitFacesFromVerticalRow<float>(data, m_width, m_height, channels, faces);
            
            // Vertical Cross
            else {
                splitFacesFromVerticalCross<float>(data, m_width, m_height, channels, faces);

                // adjust NEG_Z face
                if (_vFlip) {
                    faces[5]->flipHorizontal();
                    faces[5]->flipVertical();
                }
            }
        }
        else {

            // Equilatera
            if (m_width/2 == m_height)
                splitFacesFromEquirectangular<float>(data, m_width, m_height, channels, faces);
            
            // Horizontal Row
            else if (m_width/6 == m_height)
                splitFacesFromHorizontalRow<float>(data, m_width, m_height, channels, faces);
            
            // Horizontal Cross
            else
                splitFacesFromHorizontalCross<float>(data, m_width, m_height, channels, faces);
        }

        for (int i = 0; i < 6; i++) {
            faces[i]->upload();
            sh_samples += faces[i]->calculateSH(SH);
        }

        delete[] data;
        for(int i = 0; i < 6; ++i) {
            delete[] faces[i]->data;
            delete faces[i];
        }
        delete[] faces;

    }

    printf("SH samples: %d\n", sh_samples);

    float factor = 32.0f / (float)sh_samples;
    for (int i = 0; i < 9; i++)
        SH[i] = SH[i] * factor;

#if defined(PLATFORM_RPI) || defined(DRIVER_GBM) || defined(__EMSCRIPTEN__)
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
#endif
    
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    m_path = _path;
    m_vFlip = _vFlip;
    
    return true;
}

bool TextureCube::load(size_t _width, size_t _height, size_t _channels, const float* _data, bool _vFlip) {
    if (m_id == 0)
        glGenTextures(1, &m_id);
        
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);

    int sh_samples = 0;

    // LOAD FACES
    CubemapFace<float> **faces = new CubemapFace<float>*[6];

    if (m_height > m_width) {
        // Vertical Row
        if (m_width/6 == m_height)
            splitFacesFromVerticalRow<float>(_data, m_width, m_height, _channels, faces);
        
        // Vertical Cross
        else {
            splitFacesFromVerticalCross<float>(_data, m_width, m_height, _channels, faces);

            // adjust NEG_Z face
            if (_vFlip) {
                faces[5]->flipHorizontal();
                faces[5]->flipVertical();
            }
        }
    }
    else {

        // Equilatera
        if (m_width/2 == m_height)
            splitFacesFromEquirectangular<float>(_data, m_width, m_height, _channels, faces);
        
        // Horizontal Row
        else if (m_width/6 == m_height)
            splitFacesFromHorizontalRow<float>(_data, m_width, m_height, _channels, faces);
        
        // Horizontal Cross
        else
            splitFacesFromHorizontalCross<float>(_data, m_width, m_height, _channels, faces);
    }

    for (int i = 0; i < 6; i++) {
        faces[i]->upload();
        sh_samples += faces[i]->calculateSH(SH);
    }

    for(int i = 0; i < 6; ++i) {
        delete[] faces[i]->data;
        delete faces[i];
    }
    delete[] faces;

    float factor = 32.0f / (float)sh_samples;
    for (int i = 0; i < 9; i++)
        SH[i] = SH[i] * factor;

#if defined(PLATFORM_RPI) || defined(DRIVER_GBM) || defined(__EMSCRIPTEN__)
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
#endif
    
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    m_path = "from_memory";
    m_vFlip = _vFlip;
    
    return true;    
}

bool TextureCube::load(SkyData* _sky, int _width, bool _vFlip) {

    if (m_id == 0)
        glGenTextures(1, &m_id);

    glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);

    int sh_samples = 0;

    m_width = _width;
    m_height = int(_width/2);

    // float *data = equirectangularSkyBox(_sky, m_width, m_height);

    // // LOAD FACES
    // CubemapFace<float> **faces = new CubemapFace<float>*[6];
    // splitFacesFromEquirectangular<float>(data, m_width, m_height, faces);

    CubemapFace<float> **faces = skyCubemap(_sky, m_width, _vFlip);
    
    for (int i = 0; i < 6; i++) {
        faces[i]->upload();
        sh_samples += faces[i]->calculateSH(SH);
    }

    for(int i = 0; i < 6; ++i) {
        delete[] faces[i]->data;
        delete faces[i];
    }
    delete[] faces;
    // delete[] data;

    float factor = 32.0f / (float)sh_samples;
    for (int i = 0; i < 9; i++)
        SH[i] = SH[i] * factor;

#if defined(PLATFORM_RPI) || defined(DRIVER_GBM) || defined(__EMSCRIPTEN__)
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
#endif

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return true;
}

void TextureCube::bind() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
}

}