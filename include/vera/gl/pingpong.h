#pragma once

#include "fbo.h"

namespace vera {

class PingPong {
public:
    PingPong();
    virtual ~PingPong();

    virtual void    allocate(int _width, int _height, FboType _type, TextureFilter _filter = LINEAR, TextureWrap _wrap = REPEAT, bool _autoclear = true);
    virtual void    swap();
    virtual void    clear(float _alpha = 0.0);

    virtual bool    isAllocated() const {return m_fbos[0].isAllocated(); }
    virtual int     getWidth() const { return m_fbos[0].getWidth(); };
    virtual int     getHeight() const { return m_fbos[0].getHeight(); };

    Fbo& operator[](int n){ return m_fbos[n]; }
    Fbo& buffer(int n){ return m_fbos[n]; }

    Fbo *src;       // Source       ->  Ping
    Fbo *dst;       // Destination  ->  Pong

protected:
    Fbo m_fbos[2];    // Real addresses of ping/pong FBO´s
    int m_flag;       // Integer for making a quick swap
};

}