#pragma once

#include <string>
#include <map>

#include "gl.h"
#include "textureProps.h"
#include "../types/image.h"

// Texture — OpenGL 2D texture wrapper.
// Holds a single GL texture id and its metadata (width, height, format, filter
// mode, wrap mode).  load() is overloaded for file paths, Image objects and raw
// pixel buffers; update() allows partial pixel uploads without reallocating
// GPU storage when the dimensions and format are unchanged.
// Subclasses (TextureBump, TextureCube, TextureStream*) extend for bump maps,
// cube-maps and streaming sources.

namespace vera {

enum TextureType {
    JUST_TEXTURE = 0, BUMP_TEXTURE,
    CUBE_TEXTURE,
    STREAM_SEQUENCE_TEXTURE, STREAM_VIDEO_TEXTURE,
    STREAM_AUDIO_TEXTURE,
};

class Texture {
public:
    Texture();
    Texture(const Image& _img, TextureFilter _filter = LINEAR, TextureWrap _wrap = REPEAT);
    Texture(const Image* _img, TextureFilter _filter = LINEAR, TextureWrap _wrap = REPEAT);
    virtual ~Texture();

    virtual bool    load(const Image& _img, TextureFilter _filter = LINEAR, TextureWrap _wrap = REPEAT);
    virtual bool    load(const Image* _img, TextureFilter _filter = LINEAR, TextureWrap _wrap = REPEAT);
    virtual bool    load(const std::string& _filepath, bool _vFlip = false, TextureFilter _filter = LINEAR, TextureWrap _wrap = REPEAT);
    virtual bool    load(int _width, int _height, GLuint _id, TextureFilter _filter = LINEAR, TextureWrap _wrap = REPEAT );
    virtual bool    load(int _width, int _height, int _component, int _bits, const void* _data, TextureFilter _filter = LINEAR, TextureWrap _wrap = REPEAT);

    virtual bool    update(int _x, int _y, int _width, int _height, const void* _data);

    virtual void    clear();

    virtual bool            loaded() const { return m_id != 0; };
    virtual const GLuint    getTextureId() const { return m_id; };
    virtual std::string     getFilePath() const { return m_path; };
    virtual int             getWidth() const { return m_width; };
    virtual int             getHeight() const { return m_height; };

    /* Bind/Unbind the texture to GPU */
    virtual void    bind();
    virtual void    unbind();

protected:
    std::string     m_path;

    int             m_width;
    int             m_height;
    GLenum          m_format;
    GLenum          m_type;
    TextureFilter   m_filter; 
    TextureWrap     m_wrap;
    bool	        m_vFlip;

    GLuint          m_id;
};

typedef std::shared_ptr<Texture>        TexturePtr;
typedef std::shared_ptr<const Texture>  TextureConstPtr;
typedef std::map<std::string, Texture*> TexturesMap;
}