#include "vera/gl/pyramid.h"
#include "vera/gl/textureProps.h"

#include <algorithm>
#include <iostream>

namespace vera {

Pyramid::Pyramid(): pass(NULL), scale(1.0), m_depth(0) {
}

Pyramid::~Pyramid() {
}

void Pyramid::allocate(int _width, int _height) {
    m_depth = log2(std::min(_width, _height)) - 1;
    m_depth = std::min((unsigned int)PYRAMID_MAX_LAYERS, m_depth);

    m_width = _width;
    m_height = _height;

    float w = _width;
    float h = _height;

    for (unsigned int i = 0; i < m_depth; i++) {
        w *= 0.5f;
        h *= 0.5f;
        m_downs[i].allocate(w, h, vera::COLOR_FLOAT_TEXTURE, vera::TextureFilter::NEAREST, vera::TextureWrap::CLAMP);
    }
    
    for (unsigned int i = 0; i < m_depth; i++) {
        w *= 2.0f;
        h *= 2.0f;
        m_ups[i].allocate(w, h, vera::COLOR_FLOAT_TEXTURE, vera::TextureFilter::NEAREST, vera::TextureWrap::CLAMP);
    }
}

void Pyramid::process(const vera::Fbo *_input) {
    unsigned int i;
    // Copy the input to the first downscale
    pass(&m_downs[0], _input, NULL, 0);

    // DOWNSCALE
    for (i = 1; i < m_depth; i++)
        pass(&m_downs[i], &(m_downs[i-1]), NULL, i);
    
    // Copy the last downscale to the first upscale
    pass(&m_ups[0], &(m_downs[m_depth-2]), &(m_downs[m_depth-1]), m_depth-1);
    
    // UPSCALE
    for (i = 1; i < m_depth-1; i++)
        pass(&m_ups[i], &(m_downs[m_depth-i-2]), &(m_ups[i-1]), m_depth-1-i);
    
    // Copy the last upscale to the output
    pass(&m_ups[m_depth-1], _input, &(m_ups[m_depth-2]), 0);
}

const vera::Fbo* Pyramid::getResult(unsigned int index) const { 
    if (index < m_depth)
        return &m_ups[m_depth - 1 - index];
    else
        return &m_downs[m_depth * 2 - index - 1];
}

}