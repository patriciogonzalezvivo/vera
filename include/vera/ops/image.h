#pragma once

#include <vector>

#include "vera/types/bvh.h"
#include "vera/types/mesh.h"
#include "vera/types/image.h"

namespace vera {

void                sqrt(       Image& _image);
void                invert(     Image& _image);
void                gamma(      Image& _image, float _gamma);
void                flip(       Image& _image);

void                remap(      Image& _image, 
                                float _in_min, float _int_max, 
                                float _out_min, float _out_max, 
                                bool _clamp );

void                autolevel(  Image& _image );

void                threshold(  Image& _image, 
                                float _threshold = 0.5f);

unsigned char*      to8bit(     const Image& _image);

Image               toNormalmap(const Image& _heightmap, 
                                float _zScale = 100.0f);

Image               toLuma(     const Image& _image);

Image               toHeightmap(const Image& _terrariumImage);

Image               toHueRainbow(const Image& _graysale);

Image               mergeChannels(  const Image& _red, 
                                    const Image& _green, 
                                    const Image& _blue);

Image               mergeChannels(  const Image& _red, 
                                    const Image& _green, 
                                    const Image& _blue, 
                                    const Image& _alpha);

Image               addAlpha(   const Image& _rgb,
                                const Image& _alpha);

Image               toSdf(      const Image& _image, 
                                float _on = 1.0f);

Image               toSdf(      const Mesh& _mesh, 
                                float _paddingPct = 0.01f, 
                                int _resolution = 6);

Image               toSdfLayer(const BVH* _bvh, size_t _voxel_resolution, size_t _z_layer, float _refinement = 0.00125f);

// void                refineSdfLayer(const BVH* _bvh, Image& _images);
void                refineSdfLayers(const BVH* _bvh, std::vector<Image>& _images, float _dist);

Image               packSprite(const std::vector<Image>& _images);

std::vector<Image>  scaleSprite(const std::vector<Image>& _images, int _times);

std::vector<Image>  splitChannels(const Image& _image);

Image               scale(const Image& _image, int _width, int _height);
Image               fade(const Image& _A, const Image& _B, float _pct);

}