#include "vera/gl/fbo.h"
#include <iostream>

#include "glm/gtc/round.hpp"
#include "vera/ops/string.h"
#include "vera/window.h"

namespace vera {

Fbo::Fbo():
    scale(1.0),
    enabled(true),
    m_id(0), 
    m_fbo_id(0), m_old_fbo_id(0), 
    m_depth_id(0), m_depth_buffer(0),  
    m_type(COLOR_TEXTURE), m_width(0), m_height(0), 
    m_allocated(false), m_binded(false), m_depth(false) {
}

Fbo::~Fbo() {
    unbind();
    if (m_allocated) {
        glDeleteTextures(1, &m_id);
        glDeleteRenderbuffers(1, &m_depth_buffer);
        glDeleteFramebuffers(1, &m_fbo_id);
        m_allocated = false;
    }
}

void Fbo::allocate(const uint32_t _width, const uint32_t _height, FboType _type, TextureFilter _filter, TextureWrap _wrap, bool _autoclear) {
    bool color_texture = true;
    bool depth_texture = false;
    
    m_autoclear = _autoclear;
    
    switch(_type) {
        case COLOR_TEXTURE:
        case COLOR_FLOAT_TEXTURE:
            m_depth = false;
            color_texture = true;
            depth_texture = false;
        break;
        case GBUFFER_TEXTURE:
            _filter = NEAREST;
            _wrap = CLAMP;
        case COLOR_TEXTURE_DEPTH_BUFFER:
            m_depth = true;
            color_texture = true;
            depth_texture = false;
        break;
        case COLOR_DEPTH_TEXTURES:
            m_depth = true;
            color_texture = true;
            depth_texture = true;
        break;
        case DEPTH_TEXTURE:
            m_depth = true;
            color_texture = false;
            depth_texture = true;
        break;
    }

    if (!m_allocated) {
        // Create a frame buffer

        glGenFramebuffers(1, &m_fbo_id);

        // Create a texture to hold the depth buffer
        if (m_depth) 
            glGenRenderbuffers(1, &m_depth_buffer);
    }
    // If it's already declare skip
    else if (m_width == _width && m_height == _height && m_type == _type)
        return;

    m_type = _type;
    m_width = _width;
    m_height = _height;

    #if defined(__EMSCRIPTEN__)
    if ( getWebGLVersionNumber() == 1 ) {
        m_width = glm::ceilPowerOfTwo( glm::max(m_width, m_height) );
        m_height = m_width;
    } 
    #endif

    bind();
    
    if (color_texture) {

        // Generate a texture to hold the colour buffer
        if (m_id == 0) 
            glGenTextures(1, &m_id);

        // Color
        glBindTexture(GL_TEXTURE_2D, m_id);

        GLenum format = GL_RGBA;
        GLenum type = GL_UNSIGNED_BYTE;

#if defined(PLATFORM_RPI) || defined(DRIVER_DRM)
#else
        if (_type == COLOR_FLOAT_TEXTURE || 
            _type == GBUFFER_TEXTURE) {
            if ( haveExtension("OES_texture_float") ) {
                format = GL_RGBA32F;
                type = GL_FLOAT;
            }
            else if ( haveExtension("OES_texture_half_float") ) {
                format = GL_RGBA16F;
                type = GL_FLOAT;
            }
            else {
                format = GL_RGBA16;
                type = GL_UNSIGNED_BYTE;
            }
        }
#endif

        glTexImage2D(GL_TEXTURE_2D, 0, format, m_width, m_height, 0, GL_RGBA, type, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, getWrap(_wrap));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, getWrap(_wrap));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, getMinificationFilter(_filter));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, getMagnificationFilter(_filter));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_id, 0);
    }

#if !defined(DRIVER_DRM)
    // Depth Buffer
    if (m_depth) {
        glBindRenderbuffer(GL_RENDERBUFFER, m_depth_buffer);

        GLenum depth_format = GL_DEPTH_COMPONENT;
        GLenum depth_type = GL_UNSIGNED_SHORT;

#if defined(PLATFORM_RPI) || defined(DRIVER_DRM)
        depth_format = GL_DEPTH_COMPONENT16;

    #if GL_OES_depth32
        if ( haveExtension("GL_OES_depth32") )
            depth_format = GL_DEPTH_COMPONENT32_OES;
    #elif GL_OES_depth24
        if ( haveExtension("GL_OES_depth24") )
            depth_format = GL_DEPTH_COMPONENT24_OES;
    #endif

#elif defined(__EMSCRIPTEN__)
        depth_format = (getWebGLVersionNumber() == 1)? GL_DEPTH_COMPONENT : GL_DEPTH_COMPONENT16;

#else 
        depth_format = GL_DEPTH_COMPONENT32F;
        depth_type = GL_UNSIGNED_INT;

#endif
        glRenderbufferStorage(GL_RENDERBUFFER, depth_format, m_width, m_height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth_buffer);
    
        if (depth_texture) {

            // Generate a texture to hold the depth buffer
            if (m_depth_id == 0)
                glGenTextures(1, &m_depth_id);

            glBindTexture(GL_TEXTURE_2D, m_depth_id);
            glTexImage2D(GL_TEXTURE_2D, 0, depth_format, m_width, m_height, 0, GL_DEPTH_COMPONENT, depth_type, 0);

            #if defined(__EMSCRIPTEN__)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            #else
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            #endif
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depth_id, 0);
        }
    }
    #endif

    // CHECK
    GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (result == GL_FRAMEBUFFER_COMPLETE) 
        m_allocated = true;
    else 
        std::cout << "FBO: not complete " << result << std::endl;
    
    unbind();

    if (m_depth)
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void Fbo::bind() {
    if (!m_binded) {
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint *)&m_old_fbo_id);
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
        glViewport(0.0f, 0.0f, m_width, m_height);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        if (m_id != 0)
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_id, 0);

        if (m_depth_id != 0) 
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depth_id, 0);

        #if !defined(__EMSCRIPTEN__)
        if (m_autoclear) {
            if (m_depth)
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            else
                glClear(GL_COLOR_BUFFER_BIT);
        }
        #endif

        m_binded = true;
    }
}

void Fbo::unbind() {
    if (m_binded) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_old_fbo_id);
        glBindTexture(GL_TEXTURE_2D, 0);
        m_binded = false;
    }
}

}
