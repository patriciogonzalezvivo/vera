#pragma once

#include <vector>

#include "boundingBox.h"
#include "node.h"
#include "material.h"
#include "mesh.h"

#include "../gl/vbo.h"
#include "../gl/shader.h"

namespace vera {

typedef std::vector<Shader>    ShaderList;

class Model : public Node {
public:
    Model();
    Model(const std::string& _name, const Mesh& _mesh);
    Model(const std::string& _name, const Mesh& _mesh, const Material& _mat);
    virtual ~Model();

    bool            loaded() const { return m_model_vbo != nullptr; }
    void            addDefine(const std::string& _define, const std::string& _value = "");
    void            delDefine(const std::string& _define);
    void            clear();

    bool            setGeom(const Mesh& _mesh);
    void            setName(const std::string& _str);
    bool            setShader(const std::string& _fragStr, const std::string& _vertStr, bool verbose);
    bool            setMaterial(const Material& _material);

    const std::string& getName() const { return m_name; }
    Vbo*            getVbo() { return m_model_vbo; }
    Vbo*            getVboBbox() { return m_bbox_vbo; }
    float           getArea() const { return m_area; }
    Shader*         getShader() { return &m_mainShader; }
    Shader*         getShadowShader() { return &m_shadowShader; }
    Shader*         getNormalShader() { return &m_normalShader; }
    Shader*         getPositionShader() { return &m_positionShader; }
    Shader*         getBufferShader(size_t i) { return &(m_buffersShaders[i]); }
    int             getTotalBufferShaders() { return m_buffersShaders.size(); }
    const BoundingBox& getBoundingBox() const { return m_bbox; }
    
    void            render();
    void            renderShadow();
    void            renderNormal();
    void            renderPosition();
    void            renderBuffer(size_t i);
    
    void            render(Shader* _shader);
    void            renderBbox(Shader* _shader);

    void            printDefines();
    void            printVboInfo();

protected:
    Shader          m_mainShader;      // main pass shader
    Shader          m_shadowShader;     // depth only shadow pass
    Shader          m_normalShader;     // normal g buffer shader pass
    Shader          m_positionShader;   // position g buffer shader pass
    ShaderList      m_buffersShaders;  // N g buffer shaders passes
    
    Vbo*            m_model_vbo;
    Vbo*            m_bbox_vbo;

    BoundingBox     m_bbox;

    std::string     m_name;
    float           m_area;

    friend class    Label;
};

}
