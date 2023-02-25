#include "vera/ops/env.h"

#include "vera/gl/fbo.h"
#include "vera/gl/cubemapFace.h"
#include "vera/ops/math.h"
#include "vera/ops/pixel.h"

#include <math.h>

#include <iostream>
#include <cstdint>
#include <cstring>

#ifdef _WIN32
#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#endif

// #define USE_BILINEAR_INTERPOLATION
#include "ArHosekSkyModel.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#ifndef M_RPI
#define M_RPI 0.31830988618379067153f
#endif

namespace vera {

const float cubemapUV[6][3][3] = {
    { // +x face
        {  0.0f,  0.0f, -1.0f }, // u -> -z
        {  0.0f, -1.0f,  0.0f }, // v -> -y
        {  1.0f,  0.0f,  0.0f }, // +x face
    },
    { // -x face
        {  0.0f,  0.0f,  1.0f }, // u -> +z
        {  0.0f, -1.0f,  0.0f }, // v -> -y
        { -1.0f,  0.0f,  0.0f }, // -x face
    },
    { // +y face
        {  1.0f,  0.0f,  0.0f }, // u -> +x
        {  0.0f,  0.0f,  1.0f }, // v -> +z
        {  0.0f,  1.0f,  0.0f }, // +y face
    },
    { // -y face
        {  1.0f,  0.0f,  0.0f }, // u -> +x
        {  0.0f,  0.0f, -1.0f }, // v -> -z
        {  0.0f, -1.0f,  0.0f }, // -y face
    },
    { // +z face
        {  1.0f,  0.0f,  0.0f }, // u -> +x
        {  0.0f, -1.0f,  0.0f }, // v -> -y
        {  0.0f,  0.0f,  1.0f }, // +z face
    },
    { // -z face
        { -1.0f,  0.0f,  0.0f }, // u -> -x
        {  0.0f, -1.0f,  0.0f }, // v -> -y
        {  0.0f,  0.0f, -1.0f }, // -z face
    }
};


/// _u and _v should be center adressing and in [-1.0+invSize..1.0-invSize] range.
void texelCoordToVec(float* _out3f, float _u, float _v, uint8_t _faceId) {
    // out = u * s_faceUv[0] + v * s_faceUv[1] + s_faceUv[2].
    _out3f[0] = cubemapUV[_faceId][0][0] * _u + cubemapUV[_faceId][1][0] * _v + cubemapUV[_faceId][2][0];
    _out3f[1] = cubemapUV[_faceId][0][1] * _u + cubemapUV[_faceId][1][1] * _v + cubemapUV[_faceId][2][1];
    _out3f[2] = cubemapUV[_faceId][0][2] * _u + cubemapUV[_faceId][1][2] * _v + cubemapUV[_faceId][2][2];

    // Normalize.
    const float invLen = 1.0f/sqrtf(_out3f[0]*_out3f[0] + _out3f[1]*_out3f[1] + _out3f[2]*_out3f[2]);
    _out3f[0] *= invLen;
    _out3f[1] *= invLen;
    _out3f[2] *= invLen;
}

void latLongFromVec(float& _u, float& _v, const float _vec[3]) {
    const float phi = atan2f(_vec[0], _vec[2]);
    const float theta = acosf(_vec[1]);

    _u = (M_PI + phi) * (0.5f / M_PI);
    _v = theta * M_RPI;
}

template <typename T> 
void splitFacesFromVerticalCross(const T *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<T> **_faces ) {
    int faceWidth = _width / 3;
    int faceHeight = _height / 4;

    for (int i = 0; i < 6; i++) {
        _faces[i] = new CubemapFace<T>();
        _faces[i]->id = i;
        _faces[i]->data = new T[_channels * faceWidth * faceHeight];
        _faces[i]->width = faceWidth;
        _faces[i]->height = faceHeight;
        _faces[i]->currentOffset = 0;
    }

    for (int l = 0; l < _height; l++) {
        int jFace = (l - (l % faceHeight)) / faceHeight;

        for (int iFace = 0; iFace < 3; iFace++) {
            CubemapFace<T> *face = NULL;
            int offset = _channels * (faceWidth * iFace + l * _width);

            //      0   1   2   i
            //  3      -Z       
            //  2      -X 
            //  1  -Y  +Z  +Y       
            //  0      +X
            //  j
            //
            if (iFace == 2 && jFace == 1) face = _faces[0]; // POS_Y
            if (iFace == 0 && jFace == 1) face = _faces[1]; // NEG_Y
            if (iFace == 1 && jFace == 0) face = _faces[2]; // POS_X
            if (iFace == 1 && jFace == 2) face = _faces[3]; // NEG_X
            if (iFace == 1 && jFace == 1) face = _faces[4]; // POS_Z
            if (iFace == 1 && jFace == 3) face = _faces[5]; // NEG_Z

            if (face) {
                // the number of components to copy
                int n = sizeof(T) * faceWidth * _channels;

                std::memcpy(face->data + face->currentOffset, _data + offset, n);
                face->currentOffset += (_channels * faceWidth);
            }
        }
    }
}

template <typename T> 
void splitFacesFromHorizontalCross(const T *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<T> **_faces ) {
    int faceWidth = _width / 4;
    int faceHeight = _height / 3;

    for (int i = 0; i < 6; i++) {
        _faces[i] = new CubemapFace<T>();
        _faces[i]->id = i;
        _faces[i]->data = new T[_channels * faceWidth * faceHeight];
        _faces[i]->width = faceWidth;
        _faces[i]->height = faceHeight;
        _faces[i]->currentOffset = 0;
    }

    for (int l = 0; l < _height; l++) {
        int jFace = (l - (l % faceHeight)) / faceHeight;

        for (int iFace = 0; iFace < 4; iFace++) {
            CubemapFace<T> *face = NULL;
            int offset = _channels * (faceWidth * iFace + l * _width);

            //      0   1   2   3 i      
            //  2      -X 
            //  1  -Y  +Z  +Y  -Z     
            //  0      +X
            //  j
            //
            if (iFace == 2 && jFace == 1) face = _faces[0]; // POS_Y
            if (iFace == 0 && jFace == 1) face = _faces[1]; // NEG_Y
            if (iFace == 1 && jFace == 0) face = _faces[2]; // POS_X
            if (iFace == 1 && jFace == 2) face = _faces[3]; // NEG_X
            if (iFace == 1 && jFace == 1) face = _faces[4]; // POS_Z
            if (iFace == 3 && jFace == 1) face = _faces[5]; // NEG_Z

            if (face) {
                // the number of components to copy
                int n = sizeof(T) * faceWidth * _channels;

                std::memcpy(face->data + face->currentOffset, _data + offset, n);
                face->currentOffset += (_channels * faceWidth);
            }
        }
    }
}

template <typename T> 
void splitFacesFromHorizontalRow(const T *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<T> **_faces ) {
    int faceWidth = _width / 6;
    int faceHeight = _height;

    for (int i = 0; i < 6; i++) {
        _faces[i] = new CubemapFace<T>();
        _faces[i]->id = i;
        _faces[i]->data = new T[_channels * faceWidth * faceHeight];
        _faces[i]->width = faceWidth;
        _faces[i]->height = faceHeight;
        _faces[i]->currentOffset = 0;
    }

    for (int l = 0; l < _height; l++) {
        // int jFace = (l - (l % faceHeight)) / faceHeight;
        for (int iFace = 0; iFace < 6; iFace++) {
            CubemapFace<T> *face = NULL;
            int offset = _channels * (faceWidth * iFace + l * _width);

            //   0   1   2   3   4   5 i      
            //  +X  -X  +Y  -Y  +Z  -Z 
            //
            face = _faces[iFace];

            if (face) {
                // the number of components to copy
                int n = sizeof(T) * faceWidth * _channels;

                std::memcpy(face->data + face->currentOffset, _data + offset, n);
                face->currentOffset += (3 * faceWidth);
            }
        }
    }
}

// A few useful utilities from Filament
// https://github.com/google/filament/blob/master/tools/cmgen/src/CubemapSH.cpp
// 
template <typename T> 
void splitFacesFromVerticalRow(const T *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<T> **_faces ) {
    int faceWidth = _width;
    int faceHeight = _height/6;


    for (int i = 0; i < 6; i++) {
        _faces[i] = new CubemapFace<T>();
        _faces[i]->id = i;
        _faces[i]->data = new T[_channels * faceWidth * faceHeight];
        _faces[i]->width = faceWidth;
        _faces[i]->height = faceHeight;
        _faces[i]->currentOffset = 0;
    }

    for (int l = 0; l < _height; l++) {
        int jFace = (l - (l % faceHeight)) / faceHeight;
        for (int iFace = 0; iFace < 6; iFace++) {
            CubemapFace<T> *face = NULL;
            int offset = 3 * (faceWidth * iFace + l * _width);

            //   0   1   2   3   4   5 j      
            //  +X  -X  +Y  -Y  +Z  -Z 
            //
            face = _faces[jFace];

            if (face) {
                // the number of components to copy
                int n = sizeof(T) * faceWidth * _channels;

                std::memcpy(face->data + face->currentOffset, _data + offset, n);
                face->currentOffset += (_channels * faceWidth);
            }
        }
    }
}

static inline uint32_t ftou(float _f) { return uint32_t(int32_t(_f)); }

template <typename T> 
inline void vec3Mul(T* __restrict _result, const T* __restrict _a, float _b) {
    _result[0] = _a[0] * _b;
    _result[1] = _a[1] * _b;
    _result[2] = _a[2] * _b;
}

// From
// https://github.com/dariomanesku/cmft/blob/master/src/cmft/image.cpp#L3124
// 
template <typename T> 
void splitFacesFromEquirectangular(const T *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<T> **_faces ) {
    // Alloc data.
    const uint32_t faceWidth = (_height + 1)/2;
    const uint32_t faceHeight = faceWidth;

    // Get source parameters.
    const float srcWidthMinusOne  = float(int(_width-1));
    const float srcHeightMinusOne = float(int(_height-1));
    const float invfaceWidthf = 1.0f/float(faceWidth);

    for (int i = 0; i < 6; i++) {
        _faces[i] = new CubemapFace<T>();
        _faces[i]->id = i;
        _faces[i]->data = new T[_channels * faceWidth * faceHeight];
        _faces[i]->width = faceWidth;
        _faces[i]->height = faceHeight;
        _faces[i]->currentOffset = 0;

        for (uint32_t yy = 0; yy < faceHeight; ++yy) {
            T* dstRowData = &_faces[i]->data[yy * faceWidth * _channels];

            for (uint32_t xx = 0; xx < faceWidth; ++xx) {
                T* dstColumnData = &dstRowData[xx * _channels];

                // Cubemap (u,v) on current face.
                const float uu = 2.0f*xx*invfaceWidthf-1.0f;
                const float vv = 2.0f*yy*invfaceWidthf-1.0f;

                // Get cubemap vector (x,y,z) from (u,v,faceIdx).
                float vec[3];
                texelCoordToVec(vec, uu, vv, i);

                // Convert cubemap vector (x,y,z) to latlong (u,v).
                float xSrcf;
                float ySrcf;
                latLongFromVec(xSrcf, ySrcf, vec);

                // Convert from [0..1] to [0..(size-1)] range.
                xSrcf *= srcWidthMinusOne;
                ySrcf *= srcHeightMinusOne;

                // Sample from latlong (u,v).
                #ifdef USE_BILINEAR_INTERPOLATION
                    const uint32_t x0 = ftou(xSrcf);
                    const uint32_t y0 = ftou(ySrcf);
                    const uint32_t x1 = M_MIN(x0+1, _width-1);
                    const uint32_t y1 = M_MIN(y0+1, _height-1);

                    const T *src0 = &_data[y0 * _width * _channels + x0 * _channels];
                    const T *src1 = &_data[y0 * _width * _channels + x1 * _channels];
                    const T *src2 = &_data[y1 * _width * _channels + x0 * _channels];
                    const T *src3 = &_data[y1 * _width * _channels + x1 * _channels];

                    const float tx = xSrcf - float(int(x0));
                    const float ty = ySrcf - float(int(y0));
                    const float invTx = 1.0f - tx;
                    const float invTy = 1.0f - ty;

                    T p0[3];
                    T p1[3];
                    T p2[3];
                    T p3[3];
                    vec3Mul(p0, src0, invTx*invTy);
                    vec3Mul(p1, src1,    tx*invTy);
                    vec3Mul(p2, src2, invTx*   ty);
                    vec3Mul(p3, src3,    tx*   ty);

                    const T rr = p0[0] + p1[0] + p2[0] + p3[0];
                    const T gg = p0[1] + p1[1] + p2[1] + p3[1];
                    const T bb = p0[2] + p1[2] + p2[2] + p3[2];

                    dstColumnData[0] = rr;
                    dstColumnData[1] = gg;
                    dstColumnData[2] = bb;
                #else
                    const uint32_t xSrc = ftou(xSrcf);
                    const uint32_t ySrc = ftou(ySrcf);

                    dstColumnData[0] = _data[ySrc * _width * _channels + xSrc * _channels + 0];
                    dstColumnData[1] = _data[ySrc * _width * _channels + xSrc * _channels + 1];
                    dstColumnData[2] = _data[ySrc * _width * _channels + xSrc * _channels + 2];
                #endif
            }
        }
    }
}

//  DYNAMIC ENVIROMENT CUBEMAP
// -------------------------------------------------------------- 

glm::vec4 getFaceViewport(int _size, int _faceId) {
    if (_faceId == 0) 
        return glm::vec4((float)_size * 2.0f,   (float)_size,       (float)_size, (float)_size);
    else if (_faceId == 1) 
        return glm::vec4(0.0f,                  (float)_size,       (float)_size, (float)_size);
    else if (_faceId == 2) 
        return glm::vec4((float)_size,          (float)_size * 2.f, (float)_size, (float)_size);
    else if (_faceId == 3) 
        return glm::vec4((float)_size,          0.0f,               (float)_size, (float)_size);
    else if (_faceId == 4) 
        return glm::vec4((float)_size,          (float)_size,       (float)_size, (float)_size);
    else if (_faceId == 5)
        return glm::vec4((float)_size * 3.0f,   (float)_size,       (float)_size, (float)_size);

    return glm::vec4(0.0f, 0.0f, (float)_size, (float)_size);
}

void dynamicCubemap(std::function<void(Camera&, glm::vec4&, int&)> _renderFnc, glm::vec3 _pos, int _viewSize) {

    // save the viewport for the total quilt
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    Camera  cubemapCam;
    cubemapCam.setPosition(_pos);
    cubemapCam.setFOV(90.0);
    cubemapCam.setViewport(_viewSize, _viewSize);

    // render views and copy each view to the quilt
    for (int _side = 0; _side < 6; _side++) {
        glm::vec4 vp = getFaceViewport(_viewSize, _side);
        cubemapCam.lookAt( CubemapFace<float>::getFaceDirection(_side) * -10.0f );

        glViewport(vp.x, vp.y, vp.z, vp.w);

        glEnable(GL_SCISSOR_TEST);
        glScissor(vp.x, vp.y, vp.z, vp.w);

        _renderFnc(cubemapCam, vp, _side);
    }

    // reset viewport
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

    // // restore scissor
    glDisable(GL_SCISSOR_TEST);
    glScissor(viewport[0], viewport[1], viewport[2], viewport[3]);


}

Fbo cubemapFbo;
void dynamicCubemap(std::function<void(Camera&, glm::vec4&, int&)> _renderFnc, const std::string& _file, glm::vec3 _pos, int _viewSize) {
    if (!cubemapFbo.isAllocated() || cubemapFbo.getWidth() != _viewSize || cubemapFbo.getHeight() != _viewSize)
        cubemapFbo.allocate(_viewSize * 4, _viewSize * 3, COLOR_TEXTURE_DEPTH_BUFFER);

    cubemapFbo.bind();

    dynamicCubemap(_renderFnc, _pos, _viewSize);

    cubemapFbo.unbind();

    glBindFramebuffer(GL_FRAMEBUFFER, cubemapFbo.getId());
    int width = cubemapFbo.getWidth();
    int height = cubemapFbo.getHeight();
    std::cout << width << "x" << height << std::endl;

    unsigned char* pixels = new unsigned char[width * height*4];
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    savePixels(_file, pixels, width, height);
    delete[] pixels;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


// SKY BOX GENERATOR
// -------------------------------------------------------------- 
float* skyEquirectangular(SkyData* _sky, size_t _width, size_t _height) {
    int nPixels = _width * _height * 3;
    float *data = new float[nPixels]; 

    // FILAMENT SKYGEN 
    // https://github.com/google/filament/blob/master/tools/skygen/src/main.cpp
    //
    float solarElevation = _sky->elevation;
    float sunTheta = float(HALF_PI - solarElevation);
    float sunPhi = 0.0f;
    bool normalize = true;

    ArHosekSkyModelState* skyState[9] = {
        arhosek_xyz_skymodelstate_alloc_init(_sky->turbidity, _sky->groundAlbedo.r, solarElevation),
        arhosek_xyz_skymodelstate_alloc_init(_sky->turbidity, _sky->groundAlbedo.g, solarElevation),
        arhosek_xyz_skymodelstate_alloc_init(_sky->turbidity, _sky->groundAlbedo.b, solarElevation)
    };

    glm::mat3 XYZ_sRGB = glm::mat3(
            3.2404542f, -0.9692660f,  0.0556434f,
            -1.5371385f,  1.8760108f, -0.2040259f,
            -0.4985314f,  0.0415560f,  1.0572252f
    );

    float maxSample = 0.00001f;
    for (int y = 0; y < _height; y++) {
        float v = (y + 0.5f) / _height;
        float theta = float(M_PI * v);

        if (theta > HALF_PI) 
            continue;
            
        for (int x = 0; x < _width; x++) {
            float u = (x + 0.5f) / _width;
            float phi = float(-2.0 * M_PI * u + M_PI + _sky->azimuth);
            float gamma = angleBetween(theta, phi, sunTheta, sunPhi);

            glm::vec3 sample = glm::vec3(
                arhosek_tristim_skymodel_radiance(skyState[0], theta, gamma, 0),
                arhosek_tristim_skymodel_radiance(skyState[1], theta, gamma, 1),
                arhosek_tristim_skymodel_radiance(skyState[2], theta, gamma, 2)
            );

            if (normalize)
                sample *= float(4.0 * M_PI / 683.0);

            maxSample = std::max(maxSample, sample.y);
            sample = XYZ_sRGB * sample;

            int index = (y * _width * 3) + x * 3;
            data[index] = sample.r;
            data[index + 1] = sample.g;
            data[index + 2] = sample.b;
        }
    }

    // cleanup sky data
    arhosekskymodelstate_free(skyState[0]);
    arhosekskymodelstate_free(skyState[1]);
    arhosekskymodelstate_free(skyState[2]);

    float hdrScale = 1.0f / (normalize ? maxSample : maxSample / 16.0f);

    for (int y = 0; y < _height; y++) {
        for (int x = 0; x < _width; x++) {
            int i = (y * _width) + x;
            i *= 3;

            if (y >= _height / 2) {
                data[i + 0] = _sky->groundAlbedo.r;
                data[i + 1] = _sky->groundAlbedo.g;
                data[i + 2] = _sky->groundAlbedo.b;
            }
            else {
                data[i + 0] *= hdrScale;
                data[i + 1] *= hdrScale;
                data[i + 2] *= hdrScale;
            }
        }
    }

    return data;
}

CubemapFace<float>** skyCubemap(SkyData* _sky, int _width, bool _vFlip) {
    int width = _width;
    int height = int(_width/2);

    float *data = skyEquirectangular(_sky, width, height);

    if (_vFlip)
        flipPixelsVertically<float>(data, width, height, 3);

    // LOAD FACES
    CubemapFace<float> **faces = new CubemapFace<float>*[6];
    splitFacesFromEquirectangular<float>(data, width, height, 3, faces);

    return faces;
}

template class CubemapFace<float>;
template class CubemapFace<unsigned char>;

template void splitFacesFromVerticalCross(const float *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<float> **_faces );
template void splitFacesFromVerticalCross(const unsigned char *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<unsigned char> **_faces );

template void splitFacesFromHorizontalCross(const float *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<float> **_faces );
template void splitFacesFromHorizontalCross(const unsigned char *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<unsigned char> **_faces );

template void splitFacesFromHorizontalRow(const float *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<float> **_faces );
template void splitFacesFromHorizontalRow(const unsigned char *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<unsigned char> **_faces );

template void splitFacesFromVerticalRow(const float *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<float> **_faces );
template void splitFacesFromVerticalRow(const unsigned char *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<unsigned char> **_faces );

template void splitFacesFromEquirectangular(const float *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<float> **_faces );
template void splitFacesFromEquirectangular(const unsigned char *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<unsigned char> **_faces );

}
