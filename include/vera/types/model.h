#pragma once

#include <map>

#include "boundingBox.h"
#include "node.h"
#include "material.h"
#include "mesh.h"

#include "../gl/vbo.h"
#include "../gl/shader.h"

namespace vera {

typedef std::map<std::string, Shader*>    ShaderMap;

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
    bool            setMaterial(const Material& _material);
    void            setShader(const std::string& _fragStr, const std::string& _vertStr);
    void            setBufferShader(const std::string _bufferName, const std::string& _fragStr, const std::string& _vertStr);

    const std::string&  getName() const { return m_name; }
    Vbo*                getVbo() { return m_model_vbo; }
    Vbo*                getVboBbox() { return m_bbox_vbo; }
    float               getArea() const { return m_area; }
    const BoundingBox&  getBoundingBox() const { return m_bbox; }
    Shader*             getShader() { return &mainShader; }
    Shader*             getBufferShader(const std::string& _bufferName) { return gBuffersShaders[_bufferName]; }

    void            render();
    
    void            render(Shader* _shader);
    void            renderBbox(Shader* _shader);

    void            printDefines();
    void            printVboInfo();

    Mesh            mesh;

protected:
    Shader          mainShader;         // main pass shader
    ShaderMap       gBuffersShaders;    // shaders use for gBuffers
    
    Vbo*            m_model_vbo;
    Vbo*            m_bbox_vbo;

    BoundingBox     m_bbox;

    std::string     m_name;
    float           m_area;

    friend class    Label;
};

}
