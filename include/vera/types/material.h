#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>

#include "glm/glm.hpp"

#include "../gl/shader.h"
#include "../gl/defines.h"


namespace vera {

enum MaterialPropertyType {
    VALUE = 0,
    COLOR = 1,
    TEXTURE = 2
};

class Material : public HaveDefines {
public:
    Material();
    Material(const std::string& _name);

    virtual ~Material();

    void set(const std::string& _property, Image* _image);
    void set(const std::string& _property, const std::string& _filename);
    void set(const std::string& _property, const glm::vec3& _color);
    void set(const std::string& _property, const glm::vec4& _color);
    void set(const std::string& _property, const float* _array1D, int _n);
    void set(const std::string& _property, const float _value);

    std::string     getName() const { return name; }

    bool            haveProperty(const std::string& _property) const;

    std::string     getImagePath(const std::string& _property) const;
    Image           getImage(const std::string& _property) const;
    glm::vec4       getColor(const std::string& _property, const glm::vec2& _uv) const;
    float           getValue(const std::string& _property, const glm::vec2& _uv) const;
    glm::vec4       getColor(const std::string& _property) const;
    float           getValue(const std::string& _property) const;

    void            feedProperties(Shader& _shader) const;
    int             illuminationModel;      // illum

    std::string     name;
    std::map<std::string, MaterialPropertyType> properties;

// private:

    std::map<const std::string, float>          values;
    std::map<const std::string, glm::vec4>      colors;

    std::map<const std::string, Image*>         textures;
    std::map<const std::string, std::string>    texturesPaths;

};

typedef std::shared_ptr<Material>           MaterialPtr;
typedef std::shared_ptr<const Material>     MaterialConstPtr;
typedef std::map<std::string, Material*>    MaterialsMap;

}
