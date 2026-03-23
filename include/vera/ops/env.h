#pragma once

#include <string>
#include <functional>

#include "glm/glm.hpp"

#include "../gl/cubemapFace.h"
#include "../types/camera.h"
#include "../types/sky.h"

namespace vera {

// =============================================================================
// CUBEMAP FACE SPLITTING (from various panorama formats)
// =============================================================================

/// Split vertical cross layout into cubemap faces
/// @tparam T Pixel data type (unsigned char, float, etc.)
/// @param _data Source image data
/// @param _width Source image width
/// @param _height Source image height
/// @param _channels Number of channels per pixel
/// @param _faces Output array of 6 cubemap faces (must be pre-allocated)
template <typename T> 
void        splitFacesFromVerticalCross(const T *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<T> **_faces );

/// Split horizontal cross layout into cubemap faces
/// @tparam T Pixel data type
/// @param _data Source image data
/// @param _width Source image width
/// @param _height Source image height
/// @param _channels Number of channels per pixel
/// @param _faces Output array of 6 cubemap faces (must be pre-allocated)
template <typename T> 
void        splitFacesFromHorizontalCross(const T *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<T> **_faces );

/// Split horizontal row layout (6 faces side-by-side) into cubemap faces
/// @tparam T Pixel data type
/// @param _data Source image data
/// @param _width Source image width
/// @param _height Source image height
/// @param _channels Number of channels per pixel
/// @param _faces Output array of 6 cubemap faces (must be pre-allocated)
template <typename T> 
void        splitFacesFromHorizontalRow(const T *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<T> **_faces );

/// Split vertical row layout (6 faces stacked) into cubemap faces
/// @tparam T Pixel data type
/// @param _data Source image data
/// @param _width Source image width
/// @param _height Source image height
/// @param _channels Number of channels per pixel
/// @param _faces Output array of 6 cubemap faces (must be pre-allocated)
template <typename T> 
void        splitFacesFromVerticalRow(const T *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<T> **_faces );

/// Convert equirectangular panorama to cubemap faces
/// @tparam T Pixel data type
/// @param _data Source equirectangular image data
/// @param _width Source image width
/// @param _height Source image height
/// @param _channels Number of channels per pixel
/// @param _faces Output array of 6 cubemap faces (must be pre-allocated)
template <typename T> 
void        splitFacesFromEquirectangular(const T *_data, size_t _width, size_t _height, size_t _channels, CubemapFace<T> **_faces );

// =============================================================================
// DYNAMIC CUBEMAP RENDERING
// =============================================================================

/// Render a dynamic cubemap by calling render function 6 times (once per face)
/// @param _renderFnc Rendering callback function(camera, viewport, faceIndex)
/// @param _pos Cubemap center position (default: origin)
/// @param _viewSize Resolution of each cubemap face (default: 512)
void        dynamicCubemap(std::function<void(Camera&, glm::vec4&, int&)> _renderFnc, glm::vec3 _pos = glm::vec3(0.0), int _viewSize = 512);

/// Render a dynamic cubemap and save to file
/// @param _renderFnc Rendering callback function(camera, viewport, faceIndex)
/// @param _file Output file path (format determined by extension)
/// @param _pos Cubemap center position (default: origin)
/// @param _viewSize Resolution of each cubemap face (default: 512)
void        dynamicCubemap(std::function<void(Camera&, glm::vec4&, int&)> _renderFnc, const std::string& _file, glm::vec3 _pos = glm::vec3(0.0), int _viewSize = 512);

// =============================================================================
// SKY MODEL RENDERING
// =============================================================================

/// Generate equirectangular sky image from sky model
/// @param _sky Sky model data
/// @param _width Output image width
/// @param _height Output image height
/// @return Allocated float RGB image data (caller must free)
float*                  skyEquirectangular(SkyData* _sky, size_t _width, size_t _height);

/// Generate cubemap faces from sky model
/// @param _sky Sky model data
/// @param _width Resolution of each cubemap face
/// @param _vFlip Flip faces vertically if true
/// @return Array of 6 cubemap faces (caller must free)
CubemapFace<float>**    skyCubemap(SkyData* _sky, int _width, bool _vFlip);

}