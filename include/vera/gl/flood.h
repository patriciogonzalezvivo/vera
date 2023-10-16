#pragma once

#include "pingpong.h"
#include <functional>

namespace vera {

class Flood : public PingPong {
public:
    Flood();
    virtual ~Flood();

    virtual void    allocate(int _width, int _height, FboType _type, TextureFilter _filter = LINEAR, TextureWrap _wrap = REPEAT, bool _autoclear = true);
    unsigned int    getTotalIterations() const { return m_interations; }
    
    virtual void    process();
    std::function<void(Fbo*,const Fbo*, int)> pass;

    float           scale;

private:
    size_t          m_interations;
};

}