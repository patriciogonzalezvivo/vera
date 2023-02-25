#pragma once

#include <string>
#include <functional>

#include "glm/glm.hpp"

#include "../gl/cubemapFace.h"
#include "../types/camera.h"
#include "../types/sky.h"

namespace vera {

template <typename T> 
void        splitFacesFromVerticalCross(const T *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<T> **_faces );

template <typename T> 
void        splitFacesFromHorizontalCross(const T *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<T> **_faces );

template <typename T> 
void        splitFacesFromHorizontalRow(const T *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<T> **_faces );

template <typename T> 
void        splitFacesFromVerticalRow(const T *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<T> **_faces );

template <typename T> 
void        splitFacesFromEquirectangular(const T *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<T> **_faces );


//  DYNAMIC ENVIROMENT CUBEMAP
// -------------------------------------------------------------- 
void        dynamicCubemap(std::function<void(Camera&, glm::vec4&, int&)> _renderFnc, glm::vec3 _pos = glm::vec3(0.0), int _viewSize = 512);
void        dynamicCubemap(std::function<void(Camera&, glm::vec4&, int&)> _renderFnc, const std::string& _file, glm::vec3 _pos = glm::vec3(0.0), int _viewSize = 512);

float*                  skyEquirectangular(SkyData* _sky, size_t _width, size_t _height);
CubemapFace<float>**    skyCubemap(SkyData* _sky, int _width, bool _vFlip);

}