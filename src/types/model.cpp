#include "vera/types/model.h"

#include "vera/ops/meshes.h"
#include "vera/ops/geom.h"

#include "vera/ops/string.h"
#include "vera/shaders/defaultShaders.h"

namespace vera {

Model::Model():
    m_model_vbo(nullptr), m_bbox_vbo(nullptr),
    m_name(""), m_area(0.0f) {

    addDefine("LIGHT_SHADOWMAP", "u_lightShadowMap");
    addDefine("LIGHT_SHADOWMAP_SIZE", "2048.0");
}

Model::Model(const std::string& _name, const Mesh &_mesh):
    m_model_vbo(nullptr), m_bbox_vbo(nullptr), 
    m_area(0.0f) {
    setName(_name);
    setGeom(_mesh);
}

Model::Model(const std::string& _name, const Mesh &_mesh, const Material &_mat):
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

void Model::setName(const std::string& _str) {
    if (!m_name.empty())
        delDefine( "MODEL_NAME_" + toUpper( toUnderscore( purifyString(m_name) ) ) );

    if (!_str.empty()) {
        m_name = toLower( toUnderscore( purifyString(_str) ) );
        addDefine( "MODEL_NAME_" + toUpper( m_name ) );
    }
}

void Model::addDefine(const std::string& _define, const std::string& _value) { 
    m_shade.addDefine(_define, _value); 
    m_shadow.addDefine(_define, _value); 
}

void Model::delDefine(const std::string& _define) { 
    m_shade.delDefine(_define);
    m_shadow.delDefine(_define); 
};

void Model::printDefines() {
    m_shade.printDefines();
}

void Model::printVboInfo() {
    if (m_model_vbo)
        m_model_vbo->printInfo();
}

bool Model::setGeom(const Mesh& _mesh) {
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

    addDefine("LIGHT_SHADOWMAP", "u_lightShadowMap");
    addDefine("LIGHT_SHADOWMAP_SIZE", "2048.0");

    return true;
}

bool Model::setMaterial(const Material &_material) {
    m_shade.mergeDefines(&_material);
    m_shadow.mergeDefines(&_material);
    return true;
}

bool Model::setShader(const std::string& _fragStr, const std::string& _vertStr, bool verbose) {
    if (m_shade.loaded())
        m_shade.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);

    if (m_shadow.loaded())
        m_shadow.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);

    return  m_shade.load( _fragStr, _vertStr, SHOW_MAGENTA_SHADER, verbose) && 
            m_shadow.load( getDefaultSrc(FRAG_ERROR), _vertStr); // Use magenta frag error as a passthrough frag shader.
}

void Model::render() {
    if (m_model_vbo && m_shade.loaded())
        m_model_vbo->render(&m_shade);
}

void Model::renderShadow() {
    if (m_model_vbo && m_shadow.loaded())
        m_model_vbo->render(&m_shadow);
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
