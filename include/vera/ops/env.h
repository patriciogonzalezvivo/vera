#pragma once

#include <string>
#include <functional>

#include "glm/glm.hpp"

#include "../gl/cubemapFace.h"
#include "../types/camera.h"
#include "../types/sky.h"

namespace vera {

template <typename T> 
void        splitFacesFromVerticalCross(T *_data, int _width, int _height, CubemapFace<T> **_faces );

template <typename T> 
void        splitFacesFromHorizontalCross(T *_data, int _width, int _height, CubemapFace<T> **_faces );

template <typename T> 
void        splitFacesFromHorizontalRow(T *_data, int _width, int _height, CubemapFace<T> **_faces );

template <typename T> 
void        splitFacesFromVerticalRow(T *_data, int _width, int _height, CubemapFace<T> **_faces );

template <typename T> 
void        splitFacesFromEquirectangular(T *_data, unsigned int _width, unsigned int _height, CubemapFace<T> **_faces );


//  DYNAMIC ENVIROMENT CUBEMAP
// -------------------------------------------------------------- 
void        dynamicCubemap(std::function<void(Camera&, glm::vec4&, int&)> _renderFnc, glm::vec3 _pos = glm::vec3(0.0), int _viewSize = 512);
void        dynamicCubemap(std::function<void(Camera&, glm::vec4&, int&)> _renderFnc, const std::string& _file, glm::vec3 _pos = glm::vec3(0.0), int _viewSize = 512);

float*                  skyEquirectangular(SkyData* _sky, int _width, int _height);
CubemapFace<float>**    skyCubemap(SkyData* _sky, int _width);

}