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
    m_mainShader.addDefine(_define, _value); 
    m_shadowShader.addDefine(_define, _value);
    m_normalShader.addDefine(_define, _value);
    m_positionShader.addDefine(_define, _value);
    for (size_t i = 0; i < m_buffersShaders.size(); i++)
        m_buffersShaders[i].addDefine(_define, _value);
}

void Model::delDefine(const std::string& _define) { 
    m_mainShader.delDefine(_define);
    m_shadowShader.delDefine(_define); 
    m_normalShader.delDefine(_define);
    m_positionShader.delDefine(_define);
    for (size_t i = 0; i < m_buffersShaders.size(); i++)
        m_buffersShaders[i].delDefine(_define);
};

void Model::printDefines() {
    m_mainShader.printDefines();
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
    m_mainShader.mergeDefines(&_material);
    m_shadowShader.mergeDefines(&_material);
    m_normalShader.mergeDefines(&_material);
    m_positionShader.mergeDefines(&_material);
    for (size_t i = 0; i < m_buffersShaders.size(); i++)
        m_buffersShaders[i].mergeDefines(&_material);
    return true;
}

// Count how many BUFFERS are in the shader
int countSceneBuffers(const std::string& _source) {
    // Split Source code in lines
    std::vector<std::string> lines = vera::split(_source, '\n');

    // Group results in a vector to check for duplicates
    std::vector<std::string> results;

    // Regext to search for #ifdef SCENE_BUFFER_[NUMBER], #if defined( SCENE_BUFFER_[NUMBER] ) and #elif defined( SCENE_BUFFER_[NUMBER] ) occurences
    std::regex re(R"((?:^\s*#if|^\s*#elif)(?:\s+)(defined\s*\(\s*SCENE_BUFFER_)(\d+)(?:\s*\))|(?:^\s*#ifdef\s+SCENE_BUFFER_)(\d+))");
    std::smatch match;

    // For each line search for
    for (unsigned int l = 0; l < lines.size(); l++) {

        // if there are matches
        if (std::regex_search(lines[l], match, re)) {
            // Depending the case can be in the 2nd or 3rd group
            std::string number = std::ssub_match(match[2]).str();
            if (number.size() == 0) {
                number = std::ssub_match(match[3]).str();
            }

            // Check if it's already defined
            bool already = false;
            for (unsigned int i = 0; i < results.size(); i++) {
                if (results[i] == number) {
                    already = true;
                    break;
                }
            }

            // If it's not add it
            if (!already) {
                results.push_back(number);
            }
        }
    }

    // return the number of results
    return results.size();
}

bool Model::setShader(const std::string& _fragStr, const std::string& _vertStr, bool verbose) {
    if (m_mainShader.loaded())
        m_mainShader.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);

    if (m_shadowShader.loaded())
        m_shadowShader.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);

    if (m_normalShader.loaded())
        m_normalShader.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);

    if (m_positionShader.loaded())
        m_positionShader.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);

    int sceneBuffers_total = std::max(  countSceneBuffers(_vertStr), 
                                        countSceneBuffers(_fragStr));

    bool do_some_sceneBuffers_fail = false;
    if ( sceneBuffers_total != int(m_buffersShaders.size()) ) {
        for (size_t i = 0; i < m_buffersShaders.size(); i++)
            if (m_buffersShaders[i].loaded())
                m_buffersShaders[i].detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);

        m_buffersShaders.clear();

        for (int i = 0; i < sceneBuffers_total; i++) {
            // New Shader
            m_buffersShaders.push_back( vera::Shader() );
            m_buffersShaders[i].mergeDefines(&m_mainShader);
            m_buffersShaders[i].addDefine("SCENE_BUFFER_" + vera::toString(i));
            do_some_sceneBuffers_fail += !m_buffersShaders[i].load(_fragStr, _vertStr, vera::SHOW_MAGENTA_SHADER);
        }
    }
    else
        for (size_t i = 0; i < m_buffersShaders.size(); i++)
            do_some_sceneBuffers_fail += !m_buffersShaders[i].load(_fragStr, _vertStr, vera::SHOW_MAGENTA_SHADER);

    return  m_mainShader.load( _fragStr, _vertStr, SHOW_MAGENTA_SHADER, verbose) && 
            m_shadowShader.load( getDefaultSrc(FRAG_ERROR), _vertStr) &&
            m_normalShader.load( getDefaultSrc(FRAG_NORMAL), _vertStr) &&
            m_positionShader.load( getDefaultSrc(FRAG_POSITION), _vertStr) &&
            !do_some_sceneBuffers_fail;
}

void Model::render() {
    if (m_model_vbo && m_mainShader.loaded())
        m_model_vbo->render(&m_mainShader);
}

void Model::renderShadow() {
    if (m_model_vbo && m_shadowShader.loaded())
        m_model_vbo->render(&m_shadowShader);
}

void Model::renderNormal() {
    if (m_model_vbo && m_normalShader.loaded())
        m_model_vbo->render(&m_normalShader);
}

void Model::renderPosition() {
    if (m_model_vbo && m_positionShader.loaded())
        m_model_vbo->render(&m_positionShader);
}

void Model::renderBuffer(size_t i) {
    if (m_model_vbo && i < m_buffersShaders.size()) 
        if (m_buffersShaders[i].loaded())
            m_model_vbo->render(&(m_buffersShaders[i]));
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
