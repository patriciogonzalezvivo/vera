#include <regex>

#include "vera/types/model.h"

#include "vera/ops/meshes.h"
#include "vera/ops/geom.h"

#include "vera/ops/string.h"
#include "vera/shaders/defaultShaders.h"

namespace vera {

Model::Model():
    m_model_vbo(nullptr), m_bbox_vbo(nullptr),
    m_name(""), m_area(0.0f) {
}

Model::Model(const std::string& _name, const Mesh &_mesh):
    m_model_vbo(nullptr), m_bbox_vbo(nullptr), 
    m_area(0.0f) {
    setName(_name);
    setGeom(_mesh);
}

Model::Model(const std::string& _name, const Mesh &_mesh, Material* _mat):
    m_model_vbo(nullptr), m_bbox_vbo(nullptr), 
    m_area(0.0f) {
    setName(_name);
    setGeom(_mesh);
    setMaterial(_mat);
}

Model::~Model() {
    clear();
}

void Model::clear() {
    if (m_model_vbo) {
        delete m_model_vbo;
        m_model_vbo = nullptr;
    }

    if (m_bbox_vbo) {
        delete m_bbox_vbo;
        m_bbox_vbo = nullptr;
    }
}

void Model::addDefine(const std::string& _define, const std::string& _value) { 
    mainShader.addDefine(_define, _value); 
    for (vera::ShadersMap::iterator it = gBuffersShaders.begin(); it != gBuffersShaders.end(); ++it) {
        if (it->second)
            it->second->addDefine(_define, _value);
    }
}

void Model::delDefine(const std::string& _define) { 
    mainShader.delDefine(_define);
    for (vera::ShadersMap::iterator it = gBuffersShaders.begin(); it != gBuffersShaders.end(); ++it){
        if (it->second)
            it->second->delDefine(_define);
    }

};

bool Model::setMaterial(Material* _material) {
    mainShader.mergeDefines(_material);
    for (vera::ShadersMap::iterator it = gBuffersShaders.begin(); it != gBuffersShaders.end(); ++it) {
        if (it->second)
            it->second->mergeDefines(_material);
    }
    mesh.setMaterial( _material );
    return true;
}

void Model::setName(const std::string& _str) {
    if (!m_name.empty())
        delDefine( "MODEL_NAME_" + toUpper( toUnderscore( purifyString(m_name) ) ) );

    if (!_str.empty()) {
        m_name = toLower( toUnderscore( purifyString(_str) ) );
        addDefine( "MODEL_NAME_" + toUpper( m_name ) );
    }
}

bool Model::setGeom(const Mesh& _mesh) {
    mesh = _mesh;

    // Load Geometry VBO
    m_model_vbo = new Vbo(_mesh);

    m_bbox.clean();
    for (size_t i = 0; i < _mesh.getVerticesTotal(); i++)
        m_bbox.expand( _mesh.getVertex(i) );

    m_area = glm::min(glm::length(m_bbox.min), glm::length(m_bbox.max));
    m_bbox_vbo = new Vbo( cubeCornersMesh( m_bbox, 0.25 ) );

    // Setup Shader and GEOMETRY DEFINE FLAGS
    if (_mesh.haveColors())
        addDefine("MODEL_VERTEX_COLOR", "v_color");

    if (_mesh.haveNormals())
        addDefine("MODEL_VERTEX_NORMAL", "v_normal");

    if (_mesh.haveTexCoords())
        addDefine("MODEL_VERTEX_TEXCOORD", "v_texcoord");

    if (_mesh.haveTangents())
        addDefine("MODEL_VERTEX_TANGENT", "v_tangent");

    if (_mesh.getDrawMode() == POINTS)
        addDefine("MODEL_PRIMITIVE_POINTS");
    else if (_mesh.getDrawMode() == LINES)
        addDefine("MODEL_PRIMITIVE_LINES");
    else if (_mesh.getDrawMode() == LINE_LOOP)
        addDefine("MODEL_PRIMITIVE_LINE_LOOP");
    else if (_mesh.getDrawMode() == LINE_STRIP)
        addDefine("MODEL_PRIMITIVE_LINE_STRIP");
    else if (_mesh.getDrawMode() == TRIANGLES)
        addDefine("MODEL_PRIMITIVE_TRIANGLES");
    else if (_mesh.getDrawMode() == TRIANGLE_FAN)
        addDefine("MODEL_PRIMITIVE_TRIANGLE_FAN");

    // addDefine("LIGHT_SHADOWMAP", "u_lightShadowMap");
    // addDefine("LIGHT_SHADOWMAP_SIZE", "2048.0");

    return true;
}

void Model::setShader(const std::string& _fragStr, const std::string& _vertStr) {
    mainShader.setSource(_fragStr, _vertStr);
}

void Model::setBufferShader(const std::string _name, const std::string& _fragStr, const std::string& _vertStr) {
    ShadersMap::iterator it = gBuffersShaders.find(_name);

    if (it == gBuffersShaders.end()) {
        gBuffersShaders[_name] = new Shader();
        it = gBuffersShaders.find(_name);
    }
    else
        if (it->second == nullptr)
            it->second = new Shader();

    it->second->mergeDefines(&mainShader);
    it->second->setSource( _fragStr, _vertStr);
}

void Model::printDefines() {
    mainShader.printDefines();
}

void Model::printVboInfo() {
    if (m_model_vbo)
        m_model_vbo->printInfo();
}

void Model::render() {
    if (m_model_vbo && mainShader.isLoaded())
        m_model_vbo->render(&mainShader);
}

void Model::render(Shader* _shader) {
    if (m_model_vbo)
        m_model_vbo->render(_shader);
}

void Model::renderBbox(Shader* _shader) {
    if (m_bbox_vbo)
        m_bbox_vbo->render(_shader);
}

}
