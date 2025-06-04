
#pragma once

#include <string>
#include "glm/glm.hpp"

#include "texture.h"
#include "../types/sky.h"

namespace vera {

class TextureCube : public Texture {
public:
    TextureCube();
    virtual ~TextureCube();

    virtual bool    load(SkyData* _skyData, int _width, bool _vFlip);
    virtual bool    load(const std::string &_fileName, bool _vFlip = true);
    virtual bool    load(size_t _width, size_t _height, size_t _component, const float* _data, bool _vFlip = true);

    virtual void    bind();

    glm::vec3       SH[9];
};

typedef std::shared_ptr<TextureCube>            TextureCubePtr;
typedef std::shared_ptr<const TextureCube>      TextureCubeConstPtr;
typedef std::map<std::string, TextureCube*>     TextureCubesMap;

}