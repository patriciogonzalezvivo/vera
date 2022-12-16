#pragma once

#include <memory>
#include <vector>
#include <string>

#include "glm/glm.hpp"

namespace vera {

class Image {
public:

    Image();
    Image(const Image& _mother);
    Image(int _width, int _height, int _channels);
    Image(const uint8_t* _array3D, int _height, int _width, int _channels);
    virtual     ~Image();

    virtual bool    load(const std::string& _filepath, bool _vFlip = false);
    virtual bool    save(const std::string& _filepath, bool _vFlip = false);

    virtual bool    allocate(size_t _width, size_t _height, size_t _channels);
    virtual bool    isAllocated() const { return m_data.size() != 0; }

    virtual int     getWidth() const { return m_width; }
    virtual int     getHeight() const { return m_height; };
    virtual int     getChannels() const { return m_channels; };
    virtual std::string getFilePath() const { return m_path; };

    virtual const float& at(int _index) const { return m_data[_index]; }
    virtual const float& operator[] (int _index) const { return m_data[_index]; }
    virtual float&  operator[] (int _index) { return m_data[_index]; }
    
    virtual size_t  size() const { return m_data.size(); }

    virtual size_t  getIndex(size_t _x, size_t _y) const { return (_y * m_width + _x) * m_channels; };
    virtual size_t  getIndexUV(float _u, float _v) const { return getIndex(_u * m_width, _v * m_height); }

    virtual void    set(const uint8_t* _array3D, int _height, int _width, int _channels);

    virtual void    setValue(size_t _index, float _data);
    virtual void    setValue(size_t _index, const float* _array1D, int _n);

    virtual void    setChannels(size_t _channels) { m_channels = _channels; }
    virtual void    setColor(size_t _index, const glm::vec3& _color) { setValue( _index, &_color[0], 3); }
    virtual void    setColor(size_t _index, const glm::vec4& _color) { setValue( _index, &_color[0], std::min(4, m_channels)); }

    virtual void    setColors(const float* _array2D, int _m, int _n);

    virtual void    get(uint8_t **_array3D, int *_height, int *_width, int *_channels);

    virtual float   getValue(size_t _index) const;
    virtual glm::vec4   getColor(size_t _index) const;
    
    virtual Image   operator+ (float _value) const;
    virtual Image   operator- (float _value) const;
    virtual Image   operator* (float _value) const;
    virtual Image   operator/ (float _value) const;
    
    virtual Image&  operator+= (float _value);
    virtual Image&  operator-= (float _value);
    virtual Image&  operator*= (float _value);
    virtual Image&  operator/= (float _value);

protected:
    std::string         m_path;
    std::vector<float>  m_data;
    int                 m_width;
    int                 m_height;
    int                 m_channels;

    friend class        Texture;
};

}