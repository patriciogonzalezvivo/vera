#pragma once

#include <vector>

#include "vera/types/mesh.h"
#include "vera/types/image.h"

namespace vera {

void                sqrt(Image& _image);
void                invert(Image& _image);
void                gamma(Image& _image, float _gamma);
void                flip(Image& _image);
void                remap(Image& _image, 
                            float _in_min, float _int_max, 
                            float _out_min, float _out_max, 
                            bool _clamp);
void                autolevel(Image& _image);
void                threshold(Image& _image, float _threshold = 0.5f);

unsigned char*      to8bit(const Image& _image);
Image               toNormalmap(const Image& _heightmap, float _zScale = 100.0f);
Image               toLuma(const Image& _image);
Image               toHeightmap(const Image& _terrariumImage);
Image               toHueRainbow(const Image& _graysale);

Mesh                toTerrain(  const Image& _image,
                                const float _zScale,
                                const float _maxError = 0.001f, 
                                const float _baseHeight = 0.0f,
                                const int _maxTriangles = 0, 
                                const int _maxPoints = 0 );

Image               toSdf(const Image& _image, float _on = 1.0f);
Image               toSdf(const Mesh& _mesh, int _resolution = 8);
std::vector<Image>  toSdf(const Mesh& _mesh, float _scale, bool _absolute = false);

Image               mergeChannels(const Image& _red, const Image& _green, const Image& _blue);
Image               mergeChannels(const Image& _red, const Image& _green, const Image& _blue, const Image& _alpha);
Image               addAlpha(const Image& _rgb, const Image& _alpha);
Image               packInSprite(const std::vector<Image>& _images);
std::vector<Image>  splitChannels(const Image& _image);

}