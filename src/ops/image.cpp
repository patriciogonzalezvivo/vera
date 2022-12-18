#include "vera/ops/image.h"

#include <stdio.h>
#include <cstring>
#include <vector>
#include <iostream>
#include <algorithm>
#include <thread>
#include <random>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/normal.hpp>

#include "vera/types/bvh.h"
#include "vera/ops/math.h"
#include "vera/ops/geom.h"
#include "vera/ops/string.h"
#include "vera/ops/intersection.h"

#define	RED_WEIGHT	    0.299
#define GREEN_WEIGHT	0.587
#define BLUE_WEIGHT	    0.114
#define INF             1E20

namespace vera {

void sqrt(Image& _image) {
    int total = _image.getWidth() * _image.getHeight() * _image.getChannels();
    for (int i = 0; i < total; i++)
        _image[i] = std::sqrt(_image[i]);
}

void invert(Image& _image) {
    int total = _image.getWidth() * _image.getHeight() * _image.getChannels();
    for (int i = 0; i < total; i++)
        _image[i] = 1.0f -_image[i];
}

void gamma(Image& _image, float _gamma) {
    int total = _image.getWidth() * _image.getHeight() * _image.getChannels();
    for (int i = 0; i < total; i++)
        _image[i] = std::pow(_image[i], _gamma);
}

void autolevel(Image& _image){
    float lo = 1.0f;
    float hi = 0.0f;

    int total = _image.getWidth() * _image.getHeight() * _image.getChannels();
    for (int i = 0; i < total; i++) {
        float data = _image[i];
        lo = std::min(lo, data);
        hi = std::max(hi, data);
    }

    for (int i = 0; i < total; i++) {
        float data = _image[i];
        lo = std::min(lo, data);
        hi = std::max(hi, data);
    }

    if (hi == lo) {
        return;
    }

    for (int i = 0; i < total; i++)
        _image[i] =  (_image[i] - lo) / (hi - lo);
}

void flip(Image& _image) {
    const size_t stride = _image.getWidth() * _image.getChannels();
    float *row = (float*)malloc(stride * sizeof(float));
    float *low = &_image[0];
    float *high = &_image[(_image.getHeight() - 1) * stride];
    for (; low < high; low += stride, high -= stride) {
        std::memcpy(row, low, stride * sizeof(float));
        std::memcpy(low, high, stride * sizeof(float));
        std::memcpy(high, row, stride * sizeof(float));
    }
    free(row);
}

void remap(Image& _image, float _in_min, float _int_max, float _out_min, float _out_max, bool _clamp) {
    int total = _image.getWidth() * _image.getHeight() * _image.getChannels();
    for (int i = 0; i < total; i++)
        _image[i] = remap(_image[i], _in_min, _int_max, _out_min, _out_max, _clamp);
} 

void threshold(Image& _image, float _threshold) {
    int total = _image.getWidth() * _image.getHeight() * _image.getChannels();
    for (int i = 0; i < total; i++)
        _image[i] = (_image[i] >= _threshold)? 1.0f : 0.0f;
}

Image mergeChannels(const Image& _red, const Image& _green, const Image& _blue) {
    if (_red.getChannels() > 1 ||
        _green.getChannels() > 1 ||
        _blue.getChannels() > 1 ) {
        std::cout << "ERROR: mergeChannel() arguments must be all grayscale images" << std::endl;
        return _red;
    }

    if (_red.getWidth() != _green.getWidth() || _red.getWidth() != _blue.getWidth() ||
        _red.getHeight() != _green.getHeight() || _red.getHeight() != _blue.getHeight()) {
        std::cout << "ERROR: mergeChannel() all images must be the same size" << std::endl;
        return _green;
    }
    int width = _red.getWidth();
    int height = _red.getHeight();
    Image out = Image(width, height, 3);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = _red.getIndex(x, y);
            out.setColor(   out.getIndex(x,y), 
                            glm::vec3(_red.getValue(index), _green.getValue(index), _blue.getValue(index)));
        }
    }
    return out;
}

Image mergeChannels(const Image& _red, const Image& _green, const Image& _blue, const Image& _alpha) {
    if (_red.getChannels() > 1 ||
        _green.getChannels() > 1 ||
        _blue.getChannels() > 1 ||
        _alpha.getChannels() > 1 ) {
        std::cout << "ERROR: mergeChannel() arguments must be all grayscale images" << std::endl;
        return _red;
    }

    if (_red.getWidth() != _green.getWidth() || _red.getWidth() != _blue.getWidth() || _red.getWidth() != _alpha.getWidth() ||
        _red.getHeight() != _green.getHeight() || _red.getHeight() != _blue.getHeight() || _red.getHeight() != _alpha.getHeight()) {
        std::cout << "ERROR: mergeChannel() all images must be the same size" << std::endl;
        return _green;
    }
    int width = _red.getWidth();
    int height = _red.getHeight();
    Image out = Image(width, height, 4);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = _red.getIndex(x, y);
            out.setColor(   out.getIndex(x,y), 
                            glm::vec4(_red.getValue(index), _green.getValue(index), _blue.getValue(index), _alpha.getValue(index) ));
        }
    }
    return out;
}

Image addAlpha(const Image& _rgb, const Image& _alpha) {
    if (_rgb.getChannels() != 3 || _alpha.getChannels() > 1) {
        std::cout << "ERROR: addAlpha() first arguments must have 3 channels and the second argument must be a grayscale images" << std::endl;
        return _rgb;
    }

    if (_rgb.getWidth() != _alpha.getWidth() ||
        _rgb.getHeight() != _alpha.getHeight()) {
        std::cout << "ERROR: addAlpha() all images must be the same size" << std::endl;
        return _rgb;
    }

    int width = _rgb.getWidth();
    int height = _rgb.getHeight();
    Image out = Image(width, height, 4);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = _rgb.getIndex(x, y);
            out.setColor(   out.getIndex(x,y), 
                            _rgb.getColor(index) * glm::vec4(1.0f, 1.0f, 1.0f, _alpha.getValue(index) ));
        }
    }
    return out;
}

std::vector<Image> splitChannels(const Image& _image) {
    std::vector<Image> out;

    int width = _image.getWidth();
    int height = _image.getHeight();
    int channels = _image.getChannels();

    for (int i = 0; i < channels; i++) {
        Image channel = Image(width, height, 1);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int index = _image.getIndex(x, y);
                glm::vec4 color = _image.getColor(_image.getIndex(x, y));
                channel.setValue(channel.getIndex(x,y), color[i]);
            }
        }
        out.push_back(channel);
    }
    return out;
}

/*
Copyright (C) 2006 Pedro Felzenszwalb ( https://cs.brown.edu/people/pfelzens/dt/ )

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

/* dt of 1d function using squared distance */
static float *dt(float *f, int n) {
    float *d = new float[n];
    int *v = new int[n];
    float *z = new float[n+1];
    int k = 0;
    v[0] = 0;
    z[0] = -INF;
    z[1] = +INF;
    for (int q = 1; q <= n-1; q++) {
        float s  = ((f[q]+square(q))-(f[v[k]]+square(v[k])))/(2*q-2*v[k]);
        while (s <= z[k]) {
            k--;
            s  = ((f[q]+square(q))-(f[v[k]]+square(v[k])))/(2*q-2*v[k]);
        }
        k++;
        v[k] = q;
        z[k] = s;
        z[k+1] = +INF;
    }

    k = 0;
    for (int q = 0; q <= n-1; q++) {
        while (z[k+1] < q)
        k++;
        d[q] = square(q-v[k]) + f[v[k]];
    }

    delete [] v;
    delete [] z;
    return d;
}

/* dt of 2d function using squared distance */
void sdf(Image& _image) {
    if (_image.getChannels() > 1) {
        std::cout << "We need a one channel image to compute an SDF" << std::endl;
        return;
    }

    int width = _image.getWidth();
    int height = _image.getHeight();
    float *f = new float[std::max(width, height)];

    // transform along columns
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++)
            f[y] = _image[y * width + x];
        
        float *d = dt(f, height);
        for (int y = 0; y < height; y++) 
            _image[y * width + x] = d[y];

        delete [] d;
    }

    // transform along rows
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++)
            f[x] = _image[y * width + x];
        
        float *d = dt(f, width);
        for (int x = 0; x < width; x++)
            _image[y * width + x] = d[x];
            
        delete [] d;
    }

    sqrt(_image);
    autolevel(_image);

    delete [] f;
}


/* dt of binary image using squared distance */
Image toSdf(const Image& _image, float _on) {
    int width = _image.getWidth();
    int height = _image.getHeight();
    Image out = Image(width, height, 1);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if ( _image.getValue( _image.getIndex(x, y) ) == _on)
	            out.setValue( out.getIndex(x, y), 0.0f);
            else
                out.setValue( out.getIndex(x, y), INF);
        }
    }

    sdf(out);
    return out;
}

Image toNormalmap(const Image& _heightmap, float _zScale) {
    const int w = _heightmap.getWidth() - 1;
    const int h = _heightmap.getHeight() - 1;
    std::vector<glm::vec3> result(w * h);
    int i = 0;
    for (int y0 = 0; y0 < h; y0++) {
        const int y1 = y0 + 1;
        const float yc = y0 + 0.5f;
        for (int x0 = 0; x0 < w; x0++) {
            const int x1 = x0 + 1;
            const float xc = x0 + 0.5f;
            const float z00 = _heightmap.getValue( _heightmap.getIndex(x0, y0) ) * -_zScale;
            const float z01 = _heightmap.getValue( _heightmap.getIndex(x0, y1) ) * -_zScale;
            const float z10 = _heightmap.getValue( _heightmap.getIndex(x1, y0) ) * -_zScale;
            const float z11 = _heightmap.getValue( _heightmap.getIndex(x1, y1) ) * -_zScale;
            const float zc = (z00 + z01 + z10 + z11) / 4.f;
            const glm::vec3 p00(x0, y0, z00);
            const glm::vec3 p01(x0, y1, z01);
            const glm::vec3 p10(x1, y0, z10);
            const glm::vec3 p11(x1, y1, z11);
            const glm::vec3 pc(xc, yc, zc);
            const glm::vec3 n0 = glm::triangleNormal(pc, p00, p10);
            const glm::vec3 n1 = glm::triangleNormal(pc, p10, p11);
            const glm::vec3 n2 = glm::triangleNormal(pc, p11, p01);
            const glm::vec3 n3 = glm::triangleNormal(pc, p01, p00);
            result[i] = glm::normalize(n0 + n1 + n2 + n3) * 0.5f + 0.5f;
            i++;
        }
    }

    Image out = Image(w, h, 3);
    out.setColors(&result[0].r, result.size(), 3);
    return out;
}

Image toLuma(const Image& _image) {
    int width = _image.getWidth();
    int height = _image.getHeight();

    Image out = Image(width, height, 1);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            glm::vec4 c = _image.getColor( _image.getIndex(x, y) );
            float value = glm::dot(glm::vec3(c.x, c.y, c.z), glm::vec3(0.2126f, 0.7152f, 0.0722f));
            out.setValue( out.getIndex(x, y), value);
        }
    }

    return out;
}

Image   denoise(const Image& _color, const Image& _normal, const Image& _albedo, bool _hdr) {
#ifndef OPENIMAGEDENOISE_SUPPORT
    std::cout << "Hilma was compiled without support for OpenImageDenoise. Please install it on your system and recompile hilma" << std::endl;
    return _color;

#else
    if (_color.getChannels() != 3) {
        std::cout << "input image need to have 3 channels (RGB)" << std::endl;
        return _color;
    }

    Image out = Image(_color);

    // Create an Intel Open Image Denoise device
    oidn::DeviceRef device = oidn::newDevice();
    device.commit();

    // Create a denoising filter
    oidn::FilterRef filter = device.newFilter("RT"); // generic ray tracing filter
    filter.setImage("color", (void*)&_color[0],  oidn::Format::Float3, _color.getWidth(), _color.getHeight());
    filter.setImage("albedo", (void*)&_albedo[0], oidn::Format::Float3, _albedo.getWidth(), _albedo.getHeight()); // optional
    filter.setImage("normal", (void*)&_normal[0], oidn::Format::Float3, _normal.getWidth(), _normal.getHeight()); // optional
    filter.setImage("output", (void*)&out[0], oidn::Format::Float3, out.getWidth(), out.getHeight());
    filter.set("hdr", _hdr); // image is HDR
    filter.commit();

    // Filter the image
    filter.execute();

    // Check for errors
    const char* errorMessage;
    if (device.getError(errorMessage) != oidn::Error::None)
    std::cout << "Error: " << errorMessage << std::endl;

    return out;
#endif
}

glm::vec3 hue2rgb(float _hue) {
    float r = saturate( abs( fmod( _hue * 6.f, 6.f) - 3.f) - 1.f );
    float g = saturate( abs( fmod( _hue * 6.f + 4.f, 6.f) - 3.f) - 1.f );
    float b = saturate( abs( fmod( _hue * 6.f + 2.f, 6.f) - 3.f) - 1.f );

    #ifdef HSV2RGB_SMOOTH
    r = r*r*(3. - 2. * r);
    g = g*g*(3. - 2. * g);
    b = b*b*(3. - 2. * b);
    #endif

    return glm::vec3(r, g, b);
}

Image toHueRainbow(const Image& _in) {
    if (_in.getChannels() > 1) {
        std::cout << "The input image have more than one channel" << std::endl;
        return _in;
    }

    int width = _in.getWidth();
    int height = _in.getHeight();
    Image out = Image(width, height, 3);
    for (int y = 0; y < height; y++)
        for(int x = 0; x < width; x++)
            out.setColor( out.getIndex(x,y), hue2rgb( _in.getValue(_in.getIndex(x,y)) ) );

    return out;
}

unsigned char* to8bit(const Image& _image) {
    int total = _image.getWidth() * _image.getHeight() * _image.getChannels();
    unsigned char* pixels = new unsigned char[total];
    for (int i = 0; i < total; i++)
        pixels[i] = static_cast<char>(256 * clamp(_image[i], 0.0f, 0.999f));
    return pixels;
}

Image toHeightmap(const Image& _in) {
    int width = _in.getWidth();
    int height = _in.getHeight();
    int channels = _in.getChannels();
    Image out = Image(width, height, 1);

    for (int y = 0; y < height; y++)
    for (int x = 0; x < width; x++) {
        glm::vec4 c = _in.getColor( _in.getIndex(x, y) );
        out.setValue( out.getIndex(x, y), (c.r * 65536.0f + c.g * 256.0f + c.b) / 65536.0f );
    }

    return out;
}

// https://github.com/wxWidgets/wxWidgets/blob/eaaad6471df81ad7f801935ff63bea98bcbc715c/src/common/image.cpp#L930

struct BicubicPrecalc {
    double weight[4];
    int offset[4];
};

// The following two local functions are for the B-spline weighting of the
// bicubic sampling algorithm
double spline_cube(double value) {
    return value <= 0.0 ? 0.0 : value * value * value;
}

double spline_weight(double value) {
    return (spline_cube(value + 2) -
            4 * spline_cube(value + 1) +
            6 * spline_cube(value) -
            4 * spline_cube(value - 1)) / 6;
}

void DoCalc(BicubicPrecalc& precalc, double srcpixd, int oldDim) {
    const double dd = srcpixd - static_cast<int>(srcpixd);

    for ( int k = -1; k <= 2; k++ )
    {
        precalc.offset[k + 1] = srcpixd + k < 0.0
            ? 0
            : srcpixd + k >= oldDim
                ? oldDim - 1
                : static_cast<int>(srcpixd + k);

        precalc.weight[k + 1] = spline_weight(k - dd);
    }
}

void ResampleBicubicPrecalc(std::vector<BicubicPrecalc> &aWeight, int oldDim) {
    const int newDim = aWeight.size();
    // wxASSERT( oldDim > 0 && newDim > 0 );
    if ( newDim > 1 ) {
        // We want to map pixels in the range [0..newDim-1]
        // to the range [0..oldDim-1]
        const double scale_factor = static_cast<double>(oldDim-1) / (newDim-1);
        for ( int dstd = 0; dstd < newDim; dstd++ ) {
            // We need to calculate the source pixel to interpolate from - Y-axis
            const double srcpixd = static_cast<double>(dstd) * scale_factor;
            DoCalc(aWeight[dstd], srcpixd, oldDim);
        }
    }
    else {
        // Let's take the pixel from the center of the source image.
        const double srcpixd = static_cast<double>(oldDim - 1) / 2.0;
        DoCalc(aWeight[0], srcpixd, oldDim);
    }
}

Image scale(const Image& _image, int _width, int _height) {
    Image out = Image(_width, _height, _image.getChannels());

     // Precalculate weights
    std::vector<BicubicPrecalc> hPrecalcs(_width);
    std::vector<BicubicPrecalc> vPrecalcs(_height);

    ResampleBicubicPrecalc(hPrecalcs, _image.getWidth());
    ResampleBicubicPrecalc(vPrecalcs, _image.getHeight());

    for (int dsty = 0; dsty < _height; dsty++ ) {
        // We need to calculate the source pixel to interpolate from - Y-axis
        const BicubicPrecalc& vPrecalc = vPrecalcs[dsty];

        for (int dstx = 0; dstx < _width; dstx++ ) {
            // X-axis of pixel to interpolate from
            const BicubicPrecalc& hPrecalc = hPrecalcs[dstx];

            // Sums for each color channel
            glm::vec4 sum = glm::vec4(0.0f);

            // Here we actually determine the RGBA values for the destination pixel
            for ( int k = -1; k <= 2; k++ ) {
                // Y offset
                const int y_offset = vPrecalc.offset[k + 1];

                // Loop across the X axis
                for ( int i = -1; i <= 2; i++ ) {
                    // X offset
                    const int x_offset = hPrecalc.offset[i + 1];

                    // Calculate the exact position where the source data
                    // should be pulled from based on the x_offset and y_offset
                    int src_pixel_index = y_offset * _image.getWidth() + x_offset;

                    // Calculate the weight for the specified pixel according
                    // to the bicubic b-spline kernel we're using for
                    // interpolation
                    const float pixel_weight = vPrecalc.weight[k + 1] * hPrecalc.weight[i + 1];
                    sum += _image.getColor( _image.getIndex(x_offset, y_offset) ) * pixel_weight;
                }
            }
            out.setColor(out.getIndex(dstx, dsty), sum);
        }
    }
    return out;
}

Image mix(const Image& _A, const Image& _B, float _pct) {
    Image out = Image(_A.getWidth(), _A.getHeight(), _A.getChannels());

    if (_A.getWidth() != _B.getWidth() || _A.getHeight() != _B.getHeight() || _A.getChannels() != _B.getChannels()) {
        std::cout << "Images can't be mixed because they have different sizes (" << _A.getWidth() << "x" << _A.getHeight() << " vs " << _B.getWidth() << "x" << _B.getHeight() << ")" << std::endl;
        return out;
    }

    std::vector<std::thread> threads;
    const int nThreads = std::thread::hardware_concurrency();
    size_t pixelsPerThread = _A.getHeight() / nThreads;
    size_t pixelsLeftOver = _A.getHeight() % nThreads;
    for (int t = 0; t < nThreads; ++t) {
        size_t start = t * pixelsPerThread;
        size_t end = start + pixelsPerThread;
        if (t == nThreads - 1)
            end = start + pixelsPerThread + pixelsLeftOver;

        std::thread thrd( 
            [&out, &_A, &_B, start, end, _pct]() {
                for (size_t y = start; y < end; y++)
                for (size_t x = 0; x < _A.getWidth(); x++) {
                    size_t i = _A.getIndex(x, y);
                    out.setColor(i, glm::mix(_A.getColor(i), _B.getColor(i), _pct));
                }
            }
        );
        threads.push_back(std::move(thrd));
    }
    for (std::thread& thrd : threads)
        thrd.join();

    return out;
}

Image   toSdf(const Mesh& _mesh, float _paddingPct, int _resolution) {
    Mesh mesh = _mesh;
    center(mesh);
    std::vector<Triangle> tris = mesh.getTriangles();
    BVH acc(tris, vera::SPLIT_MIDPOINT);

    acc.square();

    glm::vec3   bdiagonal   = acc.getDiagonal();
    float       max_dist    = glm::length(bdiagonal);
    acc.expand( (max_dist*max_dist) * _paddingPct );

    int    voxel_resolution = std::pow(2, _resolution);
    float        voxel_size = 1.0/float(voxel_resolution);
    int         layersTotal = std::sqrt(voxel_resolution);
    int    image_resolution = voxel_resolution * layersTotal;

    Image rta;
    rta.allocate(image_resolution, image_resolution, 1);

    const int nThreads  = std::thread::hardware_concurrency();
    int layersPerThread = voxel_resolution / nThreads;
    int layersLeftOver  = voxel_resolution % nThreads;
    max_dist *= 0.5f;

    std::vector<std::thread> threads;
    for (int i = 0; i < nThreads; ++i) {
        int start_layer = i * layersPerThread;
        int end_layer = start_layer + layersPerThread;
        if (i == nThreads - 1)
            end_layer = start_layer + layersPerThread + layersLeftOver;

        std::thread t(
            [&acc, &rta, start_layer, end_layer, voxel_resolution, voxel_size, layersTotal, bdiagonal, max_dist ]() {
                for (int z = start_layer; z < end_layer; z++)
                for (int y = 0; y < voxel_resolution; y++)
                for (int x = 0; x < voxel_resolution; x++) {
                    // for each voxel convert it into a point in the space containing a mesh
                    glm::vec3 p = glm::vec3(x, y, z) * voxel_size;
                    p = acc.min + p * bdiagonal;

                    float c = acc.getMinSignedDistance(p);

                    size_t layerX = (z % layersTotal) * voxel_resolution; 
                    size_t layerY = floor(z / layersTotal) * voxel_resolution;
                    size_t index = rta.getIndex(layerX + x, layerY + y);
                    if (index < rta.size())
                        rta.setValue(index, glm::clamp(c/max_dist, -1.0f, 1.0f) * 0.5 + 0.5);
                }
            }
        );
        threads.push_back(std::move(t));
    }

    for (std::thread& t : threads)
        t.join();

    return rta;
}

Image toSdfLayer( const BVH* _acc, size_t _voxel_resolution, size_t _z_layer) {
    float voxel_size    = 1.0/float(_voxel_resolution);
    const int nThreads  = std::thread::hardware_concurrency();
    int layersPerThread = _voxel_resolution / nThreads;
    int layersLeftOver  = _voxel_resolution % nThreads;
    glm::vec3 bdiagonal = _acc->getDiagonal();
    float max_dist      = glm::length(bdiagonal) * 0.5f;
    Image layer = Image(_voxel_resolution, _voxel_resolution, 4);

    std::vector<std::thread> threads;
    for (int i = 0; i < nThreads; ++i) {
        int start_row = i * layersPerThread;
        int end_row = start_row + layersPerThread;
        if (i == nThreads - 1)
            end_row = start_row + layersPerThread + layersLeftOver;

        std::thread t(
            [_acc, &layer, start_row, end_row, _z_layer, _voxel_resolution, voxel_size, bdiagonal, max_dist]() {
                for (int y = start_row; y < end_row; y++)
                for (int x = 0; x < _voxel_resolution; x++) {
                    
                    glm::vec3 p = (glm::vec3(x, y, _z_layer) + 0.5f) * voxel_size;
                    p = _acc->min + p * bdiagonal;

                    float c = _acc->getMinSignedDistance(p);
                    size_t index = layer.getIndex(x, y);
                    if (index < layer.size())
                        layer.setColor(index, glm::vec4( glm::clamp(c/max_dist, -1.0f, 1.0f) * 0.5 + 0.5 ) );
                }
            }
        );
        threads.push_back(std::move(t));
    }

    for (std::thread& t : threads)
        t.join();

    return layer;
}

// void refineSdfLayer(const BVH* _bvh, Image& _images) {
//     Image org = _images;

     
// }

void refineSdfLayers(const BVH* _acc, std::vector<Image>& _images, float _dist) {

    size_t voxel_resolution = _images.size();
    float voxel_size        = 1.0/float(voxel_resolution);

    size_t triangles_total  = _acc->elements.size(); 
    glm::vec3 bdiagonal     = _acc->getDiagonal();
    float max_dist          = glm::length(bdiagonal) * 0.5f;

    std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
    std::default_random_engine generator;

    // Random vectors
    std::vector<glm::vec3> samples(64);
    for (size_t i = 0; i < 64; ++i) {
        samples[i] = glm::vec3( randomFloats(generator) * 2.0 - 1.0, 
                                randomFloats(generator) * 2.0 - 1.0, 
                                randomFloats(generator) * 2.0 - 1.0);
        samples[i] = glm::normalize(samples[i]);
        float scale = (float)i / 64.0;
        scale   = vera::lerp(0.1f, 1.0f, scale * scale);
        samples[i] *= scale;
    }

    const int nThreads      = std::thread::hardware_concurrency();
    size_t trisPerThread    = triangles_total / nThreads;
    size_t trisLeftOver     = triangles_total % nThreads;

    std::vector<std::thread> threads;
    for (size_t i = 0; i < nThreads; ++i) {
        size_t start = i * trisPerThread;
        size_t end = start + trisPerThread;
        if (i == nThreads - 1)
            end = start + trisPerThread + trisLeftOver;

        std::thread thrd(
            [_acc, &_images, &samples, start, end, voxel_resolution, max_dist, _dist]() {
                for (size_t t = start; t < end; t++) {
                    // glm::vec3 p = (glm::vec3(x, y, _z_layer) + 0.5f) * voxel_size;
                    // p = _acc->min + p * bdiagonal;

                    glm::vec3 p = _acc->elements[t].getCentroid();
                    p = p + (_acc->elements[t].getNormal() * max_dist * _dist) + samples[t%64] * max_dist * _dist * 0.5f;

                    if (_acc->contains(p)) {

                        float c = glm::clamp(_acc->getMinSignedDistance(p) / max_dist, -1.0f, 1.0f);
                        glm::ivec3 v = remap(p, _acc->min, _acc->max, glm::vec3(0.0f), glm::vec3(voxel_resolution), true);
                        size_t z = v.z % voxel_resolution;
                        size_t index = _images[z].getIndex(v.x % voxel_resolution, v.y % voxel_resolution);
                        _images[z].setColor(index, glm::vec4( c * 0.5 + 0.5 ) );
                    }
                }
            }
        );
        threads.push_back(std::move(thrd));
    }

    for (std::thread& t : threads)
        t.join();
}

Image packSprite( const std::vector<Image>& _images ) {
    Image out;

    if (_images.size() == 0)
        return out;

    // Let's asume they are all equal
    size_t layerWidth = _images[0].getWidth();
    size_t layerHeight = _images[0].getHeight();
    size_t layerChannels = _images[0].getChannels();

    size_t layers_per_side = std::ceil( std::sqrt( float(_images.size()) ));
    size_t width = layerWidth * layers_per_side;
    size_t height = layerHeight * layers_per_side;
    size_t channels = layerChannels;

    out.allocate(width, height, channels);

    std::vector<std::thread> threads;
    const int nThreads = std::thread::hardware_concurrency();
    size_t layersPerThread = _images.size() / nThreads;
    size_t layersLeftOver = _images.size() % nThreads;
    for (size_t i = 0; i < nThreads; ++i) {
        size_t start_layer = i * layersPerThread;
        size_t end_layer = start_layer + layersPerThread;
        if (i == nThreads - 1)
            end_layer = start_layer + layersPerThread + layersLeftOver;

        std::thread t(
            [&_images, &out, start_layer, end_layer, layerWidth, layerHeight, layers_per_side ]() {
                for (size_t z = start_layer; z < end_layer; z++)
                for (size_t y = 0; y < layerHeight; y++)
                for (size_t x = 0; x < layerWidth; x++) {
                    size_t layerX = (z % layers_per_side) * layerWidth; 
                    size_t layerY = floor(z / layers_per_side) * layerHeight;
                    out.setColor(   out.getIndex(layerX + x, layerY + y), 
                                    _images[z].getColor( _images[z].getIndex(x, y) ));
                }
            }
        );
        threads.push_back(std::move(t));
    }

    for (std::thread& t : threads)
        t.join();

    return out;
}

std::vector<Image>  scaleSprite(const std::vector<Image>& _in, int _times) {
    size_t in_voxel_resolution = _in.size();
    size_t out_voxel_resolution = in_voxel_resolution * _times;
    std::vector<Image> in_scaled;
    in_scaled.resize(_in.size());

    std::vector<std::thread> threads;
    const int nThreads = std::thread::hardware_concurrency();
    size_t layersPerThread = _in.size() / nThreads;
    size_t layersLeftOver = _in.size() % nThreads;
    for (size_t i = 0; i < nThreads; ++i) {
        size_t start_layer = i * layersPerThread;
        size_t end_layer = start_layer + layersPerThread;
        if (i == nThreads - 1)
            end_layer = start_layer + layersPerThread + layersLeftOver;

        std::thread t( [&in_scaled, &_in, start_layer, end_layer, out_voxel_resolution]() {
            for (size_t z = start_layer; z < end_layer; z++) {
                in_scaled[z] = vera::scale(_in[z], out_voxel_resolution, out_voxel_resolution);
            }
        });

        threads.push_back(std::move(t));
    }

    for (std::thread& t : threads)
        t.join();

    std::vector<Image> out;
    vera::Image last_layer;
    for (int z = 0; z < in_voxel_resolution; z++) {
        if (z == 0)
            out.push_back( in_scaled[z] );
        else {
            for (int i = 1; i < _times; i++)
                out.push_back( vera::mix(last_layer, in_scaled[z], float(i) / float(_times)) );
            out.push_back( in_scaled[z] );
        }

        if ( z == in_voxel_resolution - 1) {
            for (int i = 1; i < _times; i++)
                out.push_back( in_scaled[z] );
        }
        else 
            last_layer = in_scaled[z];
    }

    return out;
}

}