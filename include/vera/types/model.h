#pragma once

#include <map>

#include "boundingBox.h"
#include "node.h"
#include "material.h"

#include "mesh.h"
#ifdef SUPPORT_GSPLAT
#include "gsplat.h"
#endif

#include "../gl/vbo.h"
#include "../gl/shader.h"

namespace vera {

class Model : public Node {
public:
    Model();
#ifdef SUPPORT_GSPLAT
    Model(const std::string& _name, Gsplat* _gsplat);
#endif
    Model(const std::string& _name, const Mesh& _mesh);
    Model(const std::string& _name, const Mesh& _mesh, Material* _mat);
    virtual ~Model();

    bool            loaded() const { return m_model_vbo != nullptr; }
    void            addDefine(const std::string& _define, const std::string& _value = "");
    void            delDefine(const std::string& _define);
    void            clear();

    bool            setGeom(const Mesh& _mesh);
#ifdef SUPPORT_GSPLAT
    bool            setGeom(Gsplat* _gsplat);
#endif

    void            setName(const std::string& _str);
    bool            setMaterial(Material* _material);
    void            setShader(const std::string& _fragStr, const std::string& _vertStr);
    void            setBufferShader(const std::string _bufferName, const std::string& _fragStr, const std::string& _vertStr);

    const std::string&  getName() const { return m_name; }
#ifdef SUPPORT_GSPLAT
    Gsplat*             getGsplat() { return m_model_gsplat; }
#endif
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
    ShadersMap      gBuffersShaders;    // shaders use for gBuffers
    
    // Bounding box
    BoundingBox     m_bbox;
    Vbo*            m_bbox_vbo;

    // Model geometry
    Vbo*            m_model_vbo;
#ifdef SUPPORT_GSPLAT
    Gsplat*         m_model_gsplat;
#endif

    std::string     m_name;
    float           m_area;

    friend class    Label;
};

typedef std::shared_ptr<Model>          ModelPtr;
typedef std::shared_ptr<const Model>    ModelConstPtr;
typedef std::map<std::string, Model*>   ModelsMap;

}
