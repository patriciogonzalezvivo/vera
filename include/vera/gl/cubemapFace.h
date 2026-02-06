#pragma once

#include <math.h>

#include <iostream>
#include <cstdint>
#include <cstring>

#ifdef _WIN32
#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#endif

#include "glm/glm.hpp"
#include "gl.h"

namespace vera {

// ENVIROMENT CUBEMAPS
// -------------------------------------------------------------- 

///
///              +----------+
///              | +---->+x |
///              | |        |
///              | |  +y    |
///              |+z      2 |
///   +----------+----------+----------+----------+
///   | +---->+z | +---->+x | +---->-z | +---->-x |
///   | |        | |        | |        | |        |
///   | |  -x    | |  +z    | |  +x    | |  -z    |
///   |-y      1 |-y      4 |-y      0 |-y      5 |
///   +----------+----------+----------+----------+
///              | +---->+x |
///              | |        |
///              | |  -y    |
///              |-z      3 |
///              +----------+
///

const GLenum cubemapFaceId[6] { 
    GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 
};

const glm::vec3 cubemapDir[] = {
    glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
    glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)
};

const glm::vec3 cubemapX[] = {
    glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 1.0f),
    glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
    glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f)
};

const glm::vec3 cubemapY[] = {
    glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
    glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 1.0f),
    glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)
};

template <typename T> 
struct CubemapFace {
    static  glm::vec3 getFaceDirection(size_t _id) { return cubemapDir[_id]; }

    void    flipHorizontal() {
        int dataSize = width * height * 3;
        int n = sizeof(T) * 3 * width;
        T* newData = new T[dataSize];

        for (int i = 0; i < height; i++) {
            int offset = i * width * 3;
            int bias = -(i + 1) * 3 * width;

            memcpy(newData + dataSize + bias, data + offset, n);
        }

        delete[] data;
        data = newData;
    }

    void    flipVertical() {
        int dataSize = width * height * 3;
        int n = sizeof(T) * 3;
        T* newData = new T[dataSize];

        for(int i = 0; i < height; ++i) {
            int lineOffset = i * width * 3;

            for(int j = 0; j < width; ++j) {
                int offset = lineOffset + j * 3;
                int bias = lineOffset + width * 3 - (j + 1) * 3;

                memcpy(newData + bias, data + offset, n);
            }
        }

        delete[] data;
        data = newData;
    }

    int calculateSH(glm::vec3 *_sh) {

        // Calculate SH coefficients:
        int step = 16;
        int samples = 0;
        for (int y = 0; y < height; y += step) {
            T *p = data + y * width * 3;
            for (int x = 0; x < width; x += step) {
                glm::vec3 n = (
                    (   (cubemapX[id] * ( 2.0f * ((float)x / ((float)width - 1.0f)) - 1.0f)) +
                        (cubemapY[id] * ( -2.0f * ((float)y / ((float)height - 1.0f)) + 1.0f)) ) +
                        cubemapDir[id]); // texelDirection;
                float l = glm::length(n);
                glm::vec3 c_light = glm::vec3((float)p[0], (float)p[1], (float)p[2]);
                
                if (sizeof(T) == sizeof(char)) {
                    c_light = c_light / 255.0f;
                }
                    
                c_light = c_light * l * l * l; // texelSolidAngle * texel_radiance;
                n = glm::normalize(n);
                _sh[0] += (c_light * 0.282095f);
                _sh[1] += (c_light * -0.488603f * n.y * 2.0f / 3.0f);
                _sh[2] += (c_light * 0.488603f * n.z * 2.0f / 3.0f);
                _sh[3] += (c_light * -0.488603f * n.x * 2.0f / 3.0f);
                _sh[4] += (c_light * 1.092548f * n.x * n.y / 4.0f);
                _sh[5] += (c_light * -1.092548f * n.y * n.z / 4.0f);
                _sh[6] += (c_light * 0.315392f * (3.0f * n.z * n.z - 1.0f) / 4.0f);
                _sh[7] += (c_light * -1.092548f * n.x * n.z / 4.0f);
                _sh[8] += (c_light * 0.546274f * (n.x * n.x - n.y * n.y) / 4.0f);
                p += 3 * step;
                samples++;
            }
        }
        return samples;
    }


    void    upload() {
        GLenum type = GL_FLOAT;

        if (sizeof(T) == sizeof(char))
            type = GL_UNSIGNED_BYTE;
        
        GLenum internalFormat = GL_RGB;
        GLenum format = GL_RGB;
        const void* dataPtr = data;
        bool deleteData = false;

    #if defined(__EMSCRIPTEN__)
        if (sizeof(T) == sizeof(float)) {
            internalFormat = GL_RGBA16F;
            format = GL_RGBA;
            
            // Convert RGB to RGBA
            float* newData = new float[width * height * 4];
            float* oldData = (float*)data;
            for (int i = 0; i < width * height; i++) {
                newData[i * 4 + 0] = oldData[i * 3 + 0];
                newData[i * 4 + 1] = oldData[i * 3 + 1];
                newData[i * 4 + 2] = oldData[i * 3 + 2];
                newData[i * 4 + 3] = 1.0f;
            }
            dataPtr = newData;
            deleteData = true;
        }

    #elif defined (_WIN32)
        internalFormat = GL_RGB16F;
    #elif !defined(PLATFORM_RPI)
        internalFormat = GL_RGB;
    #endif

        glTexImage2D(cubemapFaceId[id], 0, internalFormat, width, height, 0, format, type, dataPtr);

        if (deleteData) {
            delete[] (float*)dataPtr;
        }
    }

    int     id;
	int     width;
	int     height;

	// for mem copy purposes only
	int     currentOffset;
    T       *data;
};

glm::vec3 getFaceDirection(size_t _id);

}