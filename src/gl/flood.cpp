#include <iostream>
#include <math.h>

#include "vera/gl/flood.h"

namespace vera {

Flood::Flood(): 
    scale(1.0f),
    m_interations(0){
}

Flood::~Flood() {
}

void Flood::allocate(int _width, int _height, FboType _type, TextureFilter _filter, TextureWrap _wrap, bool _autoclear) {
    // Set number of iterations based on the resolution
    m_interations = ceil(log2(std::max(_width, _height)) / log2(2.0)) + 1;

    // Allocate the FBOs
    for(int i = 0; i < 2; i++)
        m_fbos[i].allocate(_width, _height, _type, _filter, _wrap, _autoclear);

    // Clean the FBOs
    clear();

    // Set everything to 0
    m_flag = 0;
    swap();
}

void Flood::process(){
    for (size_t i = 0; i < m_interations; i++) {
        swap();
        pass(dst, src, i);
    }
}

}