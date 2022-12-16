#include "vera/ops/image.h"

#include <stdio.h>
#include <cstring>
#include <vector>
#include <iostream>

#include <map>
#include <unordered_map>
#include <algorithm>

#include <thread>
// #include <mutex>
// #include <atomic>
// #include <condition_variable>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/normal.hpp>
#include <glm/gtx/hash.hpp>

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


//  Triangulator by Michael Fogleman ( @FogleBird )
//  https://github.com/fogleman/hmm/blob/master/src/triangulator.cpp
//  All code and credits are for his genius. I took the code and make 
//  it significantly uglier using lambdas for my own conviniance
//

std::pair<glm::ivec2, float> FindCandidate( const Image& _image, const glm::ivec2 p0, const glm::ivec2 p1, const glm::ivec2 p2) { 
    
    const auto edge = []( const glm::ivec2 a, const glm::ivec2 b, const glm::ivec2 c) {
        return (b.x - c.x) * (a.y - c.y) - (b.y - c.y) * (a.x - c.x);
    };

    // triangle bounding box
    const glm::ivec2 min = glm::min(glm::min(p0, p1), p2);
    const glm::ivec2 max = glm::max(glm::max(p0, p1), p2);

    // forward differencing variables
    int w00 = edge(p1, p2, min);
    int w01 = edge(p2, p0, min);
    int w02 = edge(p0, p1, min);
    const int a01 = p1.y - p0.y;
    const int b01 = p0.x - p1.x;
    const int a12 = p2.y - p1.y;
    const int b12 = p1.x - p2.x;
    const int a20 = p0.y - p2.y;
    const int b20 = p2.x - p0.x;

    // pre-multiplied z values at vertices
    const float a = edge(p0, p1, p2);
    const float z0 = _image.getValue( _image.getIndex(p0.x, p0.y) ) / a;
    const float z1 = _image.getValue( _image.getIndex(p1.x, p1.y) ) / a;
    const float z2 = _image.getValue( _image.getIndex(p2.x, p2.y) ) / a;

    // iterate over pixels in bounding box
    float maxError = 0;
    glm::ivec2 maxPoint(0);
    for (int y = min.y; y <= max.y; y++) {
        // compute starting offset
        int dx = 0;
        if (w00 < 0 && a12 != 0)
            dx = std::max(dx, -w00 / a12);
        if (w01 < 0 && a20 != 0)
            dx = std::max(dx, -w01 / a20);
        if (w02 < 0 && a01 != 0)
            dx = std::max(dx, -w02 / a01);

        int w0 = w00 + a12 * dx;
        int w1 = w01 + a20 * dx;
        int w2 = w02 + a01 * dx;

        bool wasInside = false;

        for (int x = min.x + dx; x <= max.x; x++) {
            // check if inside triangle
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                wasInside = true;

                // compute z using barycentric coordinates
                const float z = z0 * w0 + z1 * w1 + z2 * w2;
                const float dz = std::abs(z - _image.getValue( _image.getIndex(x, y) ) );
                if (dz > maxError) {
                    maxError = dz;
                    maxPoint = glm::ivec2(x, y);
                }
            } else if (wasInside) {
                break;
            }

            w0 += a12;
            w1 += a20;
            w2 += a01;
        }

        w00 += b12;
        w01 += b20;
        w02 += b01;
    }

    if (maxPoint == p0 || maxPoint == p1 || maxPoint == p2) {
        maxError = 0;
    }

    return std::make_pair(maxPoint, maxError);
}

struct TriangulatorData {
    std::vector<glm::ivec2>  points;
    std::vector<int>        triangles;
    std::vector<int>        halfedges;

    std::vector<glm::ivec2>  candidates;
    std::vector<float>      errors;
    std::vector<int>        queueIndexes;
    std::vector<int>        queue;
    std::vector<int>        pending;

    void QueueSwap(const int i, const int j) {
        const int pi = queue[i];
        const int pj = queue[j];
        queue[i] = pj;
        queue[j] = pi;
        queueIndexes[pi] = j;
        queueIndexes[pj] = i;
    };

    bool QueueLess(const int i, const int j) {
        return -errors[queue[i]] < -errors[queue[j]];
    };

    bool QueueDown(const int i0, const int n) {
        int i = i0;
        while (1) {
            const int j1 = 2 * i + 1;
            if (j1 >= n || j1 < 0) {
                break;
            }
            const int j2 = j1 + 1;
            int j = j1;
            if (j2 < n && QueueLess(j2, j1)) {
                j = j2;
            }
            if (!QueueLess(j, i)) {
                break;
            }
            QueueSwap(i, j);
            i = j;
        }
        return i > i0;
    }

    int QueuePopBack() {
        const int t = queue.back();
        queue.pop_back();
        queueIndexes[t] = -1;
        return t;
    }

    int QueuePop() {
        const int n = queue.size() - 1;
        QueueSwap(0, n);
        QueueDown(0, n);
        return QueuePopBack();
    }

    void QueueUp(const int j0) {
        int j = j0;
        while (1) {
            int i = (j - 1) / 2;
            if (i == j || !QueueLess(j, i)) {
                break;
            }
            QueueSwap(i, j);
            j = i;
        }
    }

    void QueuePush(const int t) {
        const int i = queue.size();
        queueIndexes[t] = i;
        queue.push_back(t);
        QueueUp(i);
    }

    void QueueRemove(const int t) {
        const int i = queueIndexes[t];
        if (i < 0) {
            const auto it = std::find(pending.begin(), pending.end(), t);
            if (it != pending.end()) {
                std::swap(*it, pending.back());
                pending.pop_back();
            } 
            else {
                // this shouldn't happen!
            }
            return;
        }
        const int n = queue.size() - 1;
        if (n != i) {
            QueueSwap(i, n);
            if (!QueueDown(i, n)) {
                QueueUp(i);
            }
        }
        QueuePopBack();
    }

    int AddTriangle(const int a, const int b, const int c,
                    const int ab, const int bc, const int ca,
                    int e) {
        if (e < 0) {
            // new halfedge index
            e = triangles.size();
            // add triangle vertices
            triangles.push_back(a);
            triangles.push_back(b);
            triangles.push_back(c);
            // add triangle halfedges
            halfedges.push_back(ab);
            halfedges.push_back(bc);
            halfedges.push_back(ca);
            // add triangle metadata
            candidates.emplace_back(0);
            errors.push_back(0);
            queueIndexes.push_back(-1);
        } 
        else {
            // set triangle vertices
            triangles[e + 0] = a;
            triangles[e + 1] = b;
            triangles[e + 2] = c;
            // set triangle halfedges
            halfedges[e + 0] = ab;
            halfedges[e + 1] = bc;
            halfedges[e + 2] = ca;
        }

        // link neighboring halfedges
        if (ab >= 0)
            halfedges[ab] = e + 0;
        if (bc >= 0)
            halfedges[bc] = e + 1;
        if (ca >= 0)
            halfedges[ca] = e + 2;

        // add triangle to pending queue for later rasterization
        const int t = e / 3;
        pending.push_back(t);

        // return first halfedge index
        return e;
    }

    int AddPoint(const glm::ivec2& _point) {
        const int i = points.size();
        points.push_back(_point);
        return i;
    }

    void Flush( const Image& _image) {
        for (const int t : pending) {

            // rasterize triangle to find maximum pixel error
            const auto pair = FindCandidate(_image,
                                            points[ triangles[t*3+0] ],
                                            points[ triangles[t*3+1] ],
                                            points[ triangles[t*3+2] ]);

            // update metadata
            candidates[t] = pair.first;
            errors[t] = pair.second;

            // add triangle to priority queue
            QueuePush(t);
        }

        pending.clear();
    };

    void Legalize(const int a) {
        
        // if the pair of triangles doesn't satisfy the Delaunay condition
        // (p1 is inside the circumcircle of [p0, pl, pr]), flip them,
        // then do the same check/flip recursively for the new pair of triangles
        //
        //           pl                    pl
        //          /||\                  /  \
        //       al/ || \bl            al/    \a
        //        /  ||  \              /      \
        //       /  a||b  \    flip    /___ar___\
        //     p0\   ||   /p1   =>   p0\---bl---/p1
        //        \  ||  /              \      /
        //       ar\ || /br             b\    /br
        //          \||/                  \  /
        //           pr                    pr

        const auto inCircle = [](
            const glm::ivec2 a, const glm::ivec2 b, const glm::ivec2 c,
            const glm::ivec2 p)
        {
            const int64_t dx = a.x - p.x;
            const int64_t dy = a.y - p.y;
            const int64_t ex = b.x - p.x;
            const int64_t ey = b.y - p.y;
            const int64_t fx = c.x - p.x;
            const int64_t fy = c.y - p.y;
            const int64_t ap = dx * dx + dy * dy;
            const int64_t bp = ex * ex + ey * ey;
            const int64_t cp = fx * fx + fy * fy;
            return dx*(ey*cp-bp*fy)-dy*(ex*cp-bp*fx)+ap*(ex*fy-ey*fx) < 0;
        };

        const int b = halfedges[a];

        if (b < 0) {
            return;
        }

        const int a0 = a - a % 3;
        const int b0 = b - b % 3;
        const int al = a0 + (a + 1) % 3;
        const int ar = a0 + (a + 2) % 3;
        const int bl = b0 + (b + 2) % 3;
        const int br = b0 + (b + 1) % 3;
        const int p0 = triangles[ar];
        const int pr = triangles[a];
        const int pl = triangles[al];
        const int p1 = triangles[bl];

        if (!inCircle(points[p0], points[pr], points[pl], points[p1])) {
            return;
        }

        const int hal = halfedges[al];
        const int har = halfedges[ar];
        const int hbl = halfedges[bl];
        const int hbr = halfedges[br];

        QueueRemove(a / 3);
        QueueRemove(b / 3);

        const int t0 = AddTriangle(p0, p1, pl, -1, hbl, hal, a0);
        const int t1 = AddTriangle(p1, p0, pr, t0, har, hbr, b0);

        Legalize(t0 + 1);
        Legalize(t1 + 2);
    }
};


Mesh toTerrain( const Image& _image,
                const float _zScale,
                const float _maxError, const float _baseHeight, 
                const int _maxTriangles, const int _maxPoints) {

    if (_image.getChannels() != 1)
        return Mesh();

    TriangulatorData data;

    // add points at all four corners
    const int x0 = 0;
    const int y0 = 0;
    const int x1 = _image.getWidth() - 1;
    const int y1 = _image.getHeight() - 1;
    const int p0 = data.AddPoint(glm::ivec2(x0, y0));
    const int p1 = data.AddPoint(glm::ivec2(x1, y0));
    const int p2 = data.AddPoint(glm::ivec2(x0, y1));
    const int p3 = data.AddPoint(glm::ivec2(x1, y1));

    // add initial two triangles
    const int t0 = data.AddTriangle(p3, p0, p2, -1, -1, -1, -1);
    data.AddTriangle(p0, p3, p1, t0, -1, -1, -1);
    data.Flush(_image);

    // helper function to check if triangulation is complete
    const auto done = [&]() {
        const float e = data.errors[data.queue[0]];
        if (e <= _maxError) {
            return true;
        }
        if (_maxTriangles > 0 && data.queue.size() >= _maxTriangles) {
            return true;
        }
        if (_maxPoints > 0 && data.points.size() >= _maxPoints) {
            return true;
        }
        return e == 0;
    };

    while (!done()) {
        // pop triangle with highest error from priority queue
        const int t = data.QueuePop();

        const int e0 = t * 3 + 0;
        const int e1 = t * 3 + 1;
        const int e2 = t * 3 + 2;

        const int p0 = data.triangles[e0];
        const int p1 = data.triangles[e1];
        const int p2 = data.triangles[e2];

        const glm::ivec2 a = data.points[p0];
        const glm::ivec2 b = data.points[p1];
        const glm::ivec2 c = data.points[p2];
        const glm::ivec2 p = data.candidates[t];

        const int pn = data.AddPoint(p);

        const auto collinear = []( const glm::ivec2 p0, const glm::ivec2 p1, const glm::ivec2 p2) {
            return (p1.y-p0.y)*(p2.x-p1.x) == (p2.y-p1.y)*(p1.x-p0.x);
        };

        const auto handleCollinear = [&](const int pn, const int a) {
            const int a0 = a - a % 3;
            const int al = a0 + (a + 1) % 3;
            const int ar = a0 + (a + 2) % 3;
            const int p0 = data.triangles[ar];
            const int pr = data.triangles[a];
            const int pl = data.triangles[al];
            const int hal = data.halfedges[al];
            const int har = data.halfedges[ar];

            const int b = data.halfedges[a];

            if (b < 0) {
                const int t0 = data.AddTriangle(pn, p0, pr, -1, har, -1, a0);
                const int t1 = data.AddTriangle(p0, pn, pl, t0, -1, hal, -1);
                data.Legalize(t0 + 1);
                data.Legalize(t1 + 2);
                return;
            }

            const int b0 = b - b % 3;
            const int bl = b0 + (b + 2) % 3;
            const int br = b0 + (b + 1) % 3;
            const int p1 = data.triangles[bl];
            const int hbl = data.halfedges[bl];
            const int hbr = data.halfedges[br];

            data.QueueRemove(b / 3);

            const int t0 = data.AddTriangle(p0, pr, pn, har, -1, -1, a0);
            const int t1 = data.AddTriangle(pr, p1, pn, hbr, -1, t0 + 1, b0);
            const int t2 = data.AddTriangle(p1, pl, pn, hbl, -1, t1 + 1, -1);
            const int t3 = data.AddTriangle(pl, p0, pn, hal, t0 + 2, t2 + 1, -1);

            data.Legalize(t0);
            data.Legalize(t1);
            data.Legalize(t2);
            data.Legalize(t3);
        };

        if (collinear(a, b, p))
            handleCollinear(pn, e0);
        else if (collinear(b, c, p))
            handleCollinear(pn, e1);
        else if (collinear(c, a, p))
            handleCollinear(pn, e2);
        else {
            const int h0 = data.halfedges[e0];
            const int h1 = data.halfedges[e1];
            const int h2 = data.halfedges[e2];

            const int t0 = data.AddTriangle(p0, p1, pn, h0, -1, -1, e0);
            const int t1 = data.AddTriangle(p1, p2, pn, h1, -1, t0 + 1, -1);
            const int t2 = data.AddTriangle(p2, p0, pn, h2, t0 + 2, t1 + 1, -1);

            data.Legalize(t0);
            data.Legalize(t1);
            data.Legalize(t2);
        }

        data.Flush(_image);
    }

    std::vector<glm::vec2> texcoords;
    std::vector<glm::vec3> points;
    points.reserve(data.points.size());
    texcoords.reserve(data.points.size());
    const int w = _image.getWidth();
    const int h = _image.getHeight();
    const int w1 = w - 1;
    const int h1 = h - 1;

    for (const glm::ivec2 &p : data.points) {
        points.emplace_back(p.x, h1 - p.y, _image.getValue( _image.getIndex(p.x, p.y) ) * _zScale);
        texcoords.emplace_back(p.x/float(w1), 1.0f-p.y/float(h1));
    }

    std::vector<glm::ivec3> triangles;
    triangles.reserve(data.queue.size());
    for (const int i : data.queue) {
        triangles.emplace_back(
            data.triangles[i * 3 + 0],
            data.triangles[i * 3 + 1],
            data.triangles[i * 3 + 2] );
    }

    // BASE
    //
    
    if ( _baseHeight > 0.0f ) {

        const float z = -_baseHeight;// * _zScale;
        
        std::map<int, float> x0s;
        std::map<int, float> x1s;
        std::map<int, float> y0s;
        std::map<int, float> y1s;
        std::unordered_map<glm::vec3, int> lookup;

        // find points along each edge
        for (int i = 0; i < points.size(); i++) {
            const auto &p = points[i];
            bool edge = false;

            if (p.x == 0) {
                x0s[p.y] = p.z;
                edge = true;
            }
            else if (p.x == w1) {
                x1s[p.y] = p.z;
                edge = true;
            }

            if (p.y == 0) {
                y0s[p.x] = p.z;
                edge = true;
            }
            else if (p.y == h1) {
                y1s[p.x] = p.z;
                edge = true;
            }

            if (edge)
                lookup[p] = i;
        }

        std::vector<std::pair<int, float>> sx0s(x0s.begin(), x0s.end());
        std::vector<std::pair<int, float>> sx1s(x1s.begin(), x1s.end());
        std::vector<std::pair<int, float>> sy0s(y0s.begin(), y0s.end());
        std::vector<std::pair<int, float>> sy1s(y1s.begin(), y1s.end());

        const auto pointIndex = [&lookup, &points, &texcoords, &w1, &h1](
            const float x, const float y, const float z)
        {
            const glm::vec3 point(x, y, z);
            if (lookup.find(point) == lookup.end()) {
                lookup[point] = points.size();
                points.push_back(point);
                texcoords.push_back( glm::vec2(x/float(w1), y/float(h1)) );
            }
            return lookup[point];
        };

        // compute base center point
        const int center = pointIndex(w * 0.5f, h * 0.5f, z);

        // edge x = 0
        for (int i = 1; i < sx0s.size(); i++) {
            const int y0 = sx0s[i-1].first;
            const int y1 = sx0s[i].first;
            const float z0 = sx0s[i-1].second;
            const float z1 = sx0s[i].second;
            const int p00 = pointIndex(0, y0, z);
            const int p01 = pointIndex(0, y0, z0);
            const int p10 = pointIndex(0, y1, z);
            const int p11 = pointIndex(0, y1, z1);
            triangles.emplace_back(p01, p10, p00);
            triangles.emplace_back(p01, p11, p10);
            triangles.emplace_back(center, p00, p10);
        }

        // edge x = w1
        for (int i = 1; i < sx1s.size(); i++) {
            const int y0 = sx1s[i-1].first;
            const int y1 = sx1s[i].first;
            const float z0 = sx1s[i-1].second;
            const float z1 = sx1s[i].second;
            const int p00 = pointIndex(w1, y0, z);
            const int p01 = pointIndex(w1, y0, z0);
            const int p10 = pointIndex(w1, y1, z);
            const int p11 = pointIndex(w1, y1, z1);
            triangles.emplace_back(p00, p10, p01);
            triangles.emplace_back(p10, p11, p01);
            triangles.emplace_back(center, p10, p00);
        }

        // edge y = 0
        for (int i = 1; i < sy0s.size(); i++) {
            const int x0 = sy0s[i-1].first;
            const int x1 = sy0s[i].first;
            const float z0 = sy0s[i-1].second;
            const float z1 = sy0s[i].second;
            const int p00 = pointIndex(x0, 0, z);
            const int p01 = pointIndex(x0, 0, z0);
            const int p10 = pointIndex(x1, 0, z);
            const int p11 = pointIndex(x1, 0, z1);
            triangles.emplace_back(p00, p10, p01);
            triangles.emplace_back(p10, p11, p01);
            triangles.emplace_back(center, p10, p00);
        }

        // edge y = h1
        for (int i = 1; i < sy1s.size(); i++) {
            const int x0 = sy1s[i-1].first;
            const int x1 = sy1s[i].first;
            const float z0 = sy1s[i-1].second;
            const float z1 = sy1s[i].second;
            const int p00 = pointIndex(x0, h1, z);
            const int p01 = pointIndex(x0, h1, z0);
            const int p10 = pointIndex(x1, h1, z);
            const int p11 = pointIndex(x1, h1, z1);
            triangles.emplace_back(p01, p10, p00);
            triangles.emplace_back(p01, p11, p10);
            triangles.emplace_back(center, p00, p10);
        }
    }

    Mesh mesh;

    for (const glm::vec3 &p : points)
        mesh.addVertex( p );

    for (const glm::vec2 &t : texcoords)
        mesh.addTexCoord( t );
    
    for (const glm::ivec3 &tri : triangles)
        mesh.addTriangleIndices( tri[0], tri[1], tri[2] );

    return mesh;
}

// Mesh to SDF ( 2D texture of a 3D cube )
// 
void toSdf_layerThread( const int _start, const int _end, const int _resolution, float const _size, int const _layersTotal, 
                        const BVH *_acc, Image* _img) {

    glm::vec3   bdiagonal   = _acc->getDiagonal();
    float       max_dist    = glm::length(bdiagonal);
                max_dist    *= 0.5;

    for (int z = _start; z < _end; z++)
    for (int y = 0; y < _resolution; y++)
    for (int x = 0; x < _resolution; x++) {
        // for each voxel convert it into a point in the space containing a mesh
        glm::vec3 p = glm::vec3(x, y, z) * _size;
        p = _acc->min + p * bdiagonal;

        float c = _acc->getMinSignedDistance(p);

        size_t layerX = (z % _layersTotal) * _resolution; 
        size_t layerY = floor(z / _layersTotal) * _resolution;
        size_t index = _img->getIndex(layerX + x, layerY + y);
        if (index < _img->size())
            _img->setValue(index, glm::clamp(c/max_dist, -1.0f, 1.0f) * 0.5 + 0.5);
    }

    std::cout << "Finish layers " << _start << " to " << _end << std::endl;
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

    const int nThreads = std::thread::hardware_concurrency();
    int layersPerThread = voxel_resolution / nThreads;
    int layersLeftOver = voxel_resolution % nThreads;
    std::vector<std::thread> threads;

    for (int i = 0; i < nThreads; ++i) {
        int start_layer = i * layersPerThread;
        int end_layer = start_layer + layersPerThread;
        if (i == nThreads - 1)
            end_layer = start_layer + layersPerThread + layersLeftOver;

        std::thread t(
            [start_layer, end_layer, voxel_resolution, voxel_size, layersTotal, &acc, &rta]() {
                toSdf_layerThread(start_layer, end_layer, voxel_resolution, voxel_size, layersTotal, &acc, &rta);
            }
        );

        threads.push_back(std::move(t));
    }

    for (std::thread& t : threads)
        t.join();

    return rta;
}

void toSdf_rowThread(   const int _startY, const int _endY, const int _Z, 
                        const int _resolution, float const _size, 
                        const BVH *_acc, Image* _img ) {

    glm::vec3   bdiagonal   = _acc->getDiagonal();
    float       max_dist    = glm::length(bdiagonal);
                max_dist    *= 0.5;

    for (int y = _startY; y < _endY; y++)
    for (int x = 0; x < _resolution; x++) {
        // for each voxel convert it into a point in the space containing a mesh
        glm::vec3 p = glm::vec3(x, y, _Z) * _size;
        p = _acc->min + p * bdiagonal;

        float c = _acc->getMinSignedDistance(p);
        size_t index = _img->getIndex(x, y);
        if (index < _img->size())
            _img->setValue(index, glm::clamp(c/max_dist, -1.0f, 1.0f) * 0.5 + 0.5);
    }
}

Image toSdfLayer( const BVH* _acc, size_t _voxel_resolution, size_t _z_layer) {
    float        voxel_size = 1.0/float(_voxel_resolution);

    const int nThreads = std::thread::hardware_concurrency();
    int layersPerThread = _voxel_resolution / nThreads;
    int layersLeftOver = _voxel_resolution % nThreads;
    std::vector<std::thread> threads;

    Image rta = Image(_voxel_resolution, _voxel_resolution, 1);
    for (int i = 0; i < nThreads; ++i) {
        int start_row = i * layersPerThread;
        int end_row = start_row + layersPerThread;
        if (i == nThreads - 1)
            end_row = start_row + layersPerThread + layersLeftOver;

        std::thread t(
            [start_row, end_row, _z_layer, _voxel_resolution, voxel_size, _acc, &rta]() {
                toSdf_rowThread(  start_row, end_row, _z_layer, 
                                    _voxel_resolution, voxel_size, 
                                    _acc, &rta);
            }
        );
        threads.push_back(std::move(t));
    }

    for (std::thread& t : threads)
        t.join();

    return rta;

}

Image toHeightmap(const Image& _in) {
    int width = _in.getWidth();
    int height = _in.getHeight();
    int channels = _in.getChannels();
    Image out = Image(width, height, 1);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            glm::vec4 c = _in.getColor( _in.getIndex(x, y) );
            out.setValue( out.getIndex(x, y), (c.r * 65536.0f + c.g * 256.0f + c.b) / 65536.0f );
        }
    }

    return out;
}

Image packSprite( const std::vector<Image>& _images ) {
    Image out;

    if (_images.size() == 0)
        return out;

    // Let's asume they are all equal
    int layerWidth = _images[0].getWidth();
    int layerHeight = _images[0].getHeight();
    int layerChannels = _images[0].getChannels();

    int layers_per_side = std::ceil( std::sqrt( float(_images.size()) ));
    int width = layerWidth * layers_per_side;
    int height = layerHeight * layers_per_side;
    int channels = layerChannels;

    out.allocate(width, height, channels);
    for (int z = 0; z < _images.size(); z++)
    for (int y = 0; y < layerHeight; y++)
    for (int x = 0; x < layerWidth; x++) {
        size_t layerX = (z % layers_per_side) * layerWidth; 
        size_t layerY = floor(z / layers_per_side) * layerHeight;
        out.setColor(   out.getIndex(layerX + x, layerY + y), 
                        _images[z].getColor( _images[z].getIndex(x, y) ));
    }

    return out;
}

unsigned char* to8bit(const Image& _image) {
    int total = _image.getWidth() * _image.getHeight() * _image.getChannels();
    unsigned char* pixels = new unsigned char[total];
    for (int i = 0; i < total; i++)
        pixels[i] = static_cast<char>(256 * clamp(_image[i], 0.0f, 0.999f));
    return pixels;
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

    for ( int dsty = 0; dsty < _height; dsty++ ) {
        // We need to calculate the source pixel to interpolate from - Y-axis
        const BicubicPrecalc& vPrecalc = vPrecalcs[dsty];

        for ( int dstx = 0; dstx < _width; dstx++ ) {
            // X-axis of pixel to interpolate from
            const BicubicPrecalc& hPrecalc = hPrecalcs[dstx];

            // Sums for each color channel
            glm::vec4 sum;

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

            out.setColor( out.getIndex(dstx, dsty), sum);
        }
    }

    return out;
}

Image mix(const Image& _A, const Image& _B, float _pct) {
    Image out = Image(_A.getWidth(), _A.getHeight(), _A.getChannels());

    if (_A.getWidth() != _B.getWidth() || _A.getHeight() != _B.getHeight()) {
        std::cout << "Images can't be mixed because they have different sizes" << std::endl;
        return out;
    }

    for (size_t i = 0; i < _A.size(); i++)
        out.setColor(i, glm::mix(_A.getColor(i), _B.getColor(i), _pct ));

    return out;
}

std::vector<Image>  scaleSprite(const std::vector<Image>& _in, int _times) {
    size_t in_voxel_resolution = _in.size();
    size_t out_voxel_resolution = in_voxel_resolution * _times;
    std::vector<Image> out;
    
    vera::Image last_layer;

    for (int z = 0; z < in_voxel_resolution; z++) {
        vera::Image img = vera::scale(_in[z], out_voxel_resolution, out_voxel_resolution);

        if (z == 0) {
            out.push_back( img );
        }
        else {
            for (int i = 1; i < _times; i++)
                out.push_back( vera::mix(last_layer, img, float(i) / float(_times)) );
            out.push_back( img );
        }

        if ( z == in_voxel_resolution - 1) {
            for (int i = 1; i < _times; i++)
                out.push_back( img );
        }
        else 
            last_layer = img;
    }

    return out;
}

}