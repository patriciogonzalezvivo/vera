#include "vera/scene/material.h"
#include "vera/string.h"

namespace vera {

Material::Material():name("default") {
}

Material::~Material() {
}

void Material::feedProperties(Shader& _shader) const {
    _shader.addDefine( "MATERIAL_NAME_" + toUpper(name) );
    _shader.mergeDefines( m_defines );
}

}