#pragma once

#include <vector>

#include "vera/types/bvh.h"
#include "vera/types/mesh.h"
#include "vera/types/image.h"

// Image processing operations that work directly on Image objects.
// All functions operate in-place unless they return a new Image.
// Channel values are assumed to be floating-point in [0, 1] unless noted.

namespace vera {

// =============================================================================
// IN-PLACE IMAGE TRANSFORMATIONS
// =============================================================================

/// Apply square root to all pixel values (brightening)
/// @param _image Image to modify in-place
void                sqrt(       Image& _image);

/// Invert all pixel values (1.0 - value)
/// @param _image Image to modify in-place
void                invert(     Image& _image);

/// Apply gamma correction to all channels
/// @param _image Image to modify in-place
/// @param _gamma Gamma exponent (>1 darkens, <1 brightens)
void                gamma(      Image& _image, float _gamma);

/// Flip image vertically (swap top and bottom)
/// @param _image Image to modify in-place
void                flip(       Image& _image);

/// Remap pixel values from input range to output range
/// @param _image Image to modify in-place
/// @param _in_min Input minimum value
/// @param _int_max Input maximum value
/// @param _out_min Output minimum value
/// @param _out_max Output maximum value
/// @param _clamp Clamp output to [_out_min, _out_max]
void                remap(      Image& _image, 
                                float _in_min, float _int_max, 
                                float _out_min, float _out_max, 
                                bool _clamp );

/// Auto-level image by stretching contrast to full [0, 1] range
/// @param _image Image to modify in-place
void                autolevel(  Image& _image );

/// Apply binary threshold (values < threshold → 0, else → 1)
/// @param _image Image to modify in-place
/// @param _threshold Threshold value (default 0.5)
void                threshold(  Image& _image, 
                                float _threshold = 0.5f);

// =============================================================================
// IMAGE CONVERSION AND GENERATION
// =============================================================================

/// Convert floating-point image to 8-bit unsigned char array
/// @param _image Source image
/// @return Pointer to newly allocated 8-bit pixel data (caller must free)
unsigned char*      to8bit(     const Image& _image);

/// Generate normal map from heightmap
/// @param _heightmap Source heightmap (luminance values = height)
/// @param _zScale Height scale factor (default 100.0)
/// @return Normal map Image (normals encoded in RGB)
Image               toNormalmap(const Image& _heightmap, 
                                float _zScale = 100.0f);

/// Convert image to grayscale luminance
/// @param _image Source image
/// @return Single-channel luminance Image
Image               toLuma(     const Image& _image);

/// Convert Terrarium-encoded image to heightmap
/// 
/// Terrarium encoding: height = (R * 256 + G + B / 256) - 32768
/// @param _terrariumImage Terrarium-encoded elevation data
/// @return Heightmap Image with elevation values
Image               toHeightmap(const Image& _terrariumImage);

/// Convert grayscale to hue rainbow colormap
/// @param _graysale Grayscale source image
/// @return Rainbow-colored Image (grayscale mapped to hue)
Image               toHueRainbow(const Image& _graysale);

/// Merge three single-channel images into RGB
/// @param _red Red channel image
/// @param _green Green channel image
/// @param _blue Blue channel image
/// @return RGB Image
Image               mergeChannels(  const Image& _red, 
                                    const Image& _green, 
                                    const Image& _blue);

/// Merge four single-channel images into RGBA
/// @param _red Red channel image
/// @param _green Green channel image
/// @param _blue Blue channel image
/// @param _alpha Alpha channel image
/// @return RGBA Image
Image               mergeChannels(  const Image& _red, 
                                    const Image& _green, 
                                    const Image& _blue, 
                                    const Image& _alpha);

/// Add alpha channel to RGB image
/// @param _rgb RGB source image
/// @param _alpha Alpha channel image
/// @return RGBA Image
Image               addAlpha(   const Image& _rgb,
                                const Image& _alpha);

/// Generate signed distance field from binary image
/// @param _image Binary input image (threshold at _on value)
/// @param _on Value considered "inside" (default 1.0)
/// @return SDF Image (negative inside, positive outside)
Image               toSdf(      const Image& _image, 
                                float _on = 1.0f);

/// Generate signed distance field from 3D mesh
/// @param _mesh Input mesh
/// @param _paddingPct Padding as percentage of bounding box (default 0.01)
/// @param _resolution Resolution level (default 6)
/// @return SDF Image representing mesh interior/exterior distance
Image               toSdf(      const Mesh& _mesh, 
                                float _paddingPct = 0.01f, 
                                int _resolution = 6);

/// Generate single Z-layer of SDF from BVH (bounding volume hierarchy)
/// @param _bvh Pointer to BVH structure
/// @param _voxel_resolution Voxel grid resolution
/// @param _z_layer Z-layer index to generate
/// @param _refinement Refinement step size (default 0.00125)
/// @return SDF Image for specified layer
Image               toSdfLayer(const BVH* _bvh, size_t _voxel_resolution, size_t _z_layer, float _refinement = 0.00125f);

// void                refineSdfLayer(const BVH* _bvh, Image& _images);

/// Refine multiple SDF layers from BVH
/// @param _bvh Pointer to BVH structure
/// @param _images Vector of SDF layer images to refine (modified in-place)
/// @param _dist Distance threshold for refinement
void                refineSdfLayers(const BVH* _bvh, std::vector<Image>& _images, float _dist);

// =============================================================================
// IMAGE COMPOSITION AND MANIPULATION
// =============================================================================

/// Pack multiple images into sprite sheet (atlas)
/// @param _images Vector of images to pack
/// @return Single Image containing all sprites arranged optimally
Image               packSprite(const std::vector<Image>& _images);

/// Scale sprite sheet by integer factor
/// @param _images Vector of sprite images
/// @param _times Scale multiplier (e.g., 2 = double size)
/// @return Vector of scaled sprite images
std::vector<Image>  scaleSprite(const std::vector<Image>& _images, int _times);

/// Split multi-channel image into separate single-channel images
/// @param _image Source multi-channel image
/// @return Vector of single-channel Images (one per channel)
std::vector<Image>  splitChannels(const Image& _image);

/// Resize image to specified dimensions
/// @param _image Source image
/// @param _width Target width in pixels
/// @param _height Target height in pixels
/// @return Resized Image
Image               scale(const Image& _image, int _width, int _height);

/// Linear blend/fade between two images
/// @param _A First image
/// @param _B Second image
/// @param _pct Blend percentage (0.0 = all A, 1.0 = all B)
/// @return Blended Image
Image               fade(const Image& _A, const Image& _B, float _pct);

}