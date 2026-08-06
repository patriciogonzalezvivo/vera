#include "vera/io/gltf.h"

#include <iostream>
#include <fstream>
#include <string>
#include <map>

#include "vera/gl/vbo.h"
#include "vera/ops/fs.h"
#include "vera/ops/pixel.h"
#include "vera/ops/string.h"

#include "stb_image.h"
#include "stb_image_write.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/quaternion.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION
// #define JSON_NOEXCEPTION
#include "tiny_gltf.h"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

namespace vera {

bool loadModel(const std::string& _filename, tinygltf::Model& _model) {
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    std::string ext = getExt(_filename);

    bool res = false;

    // assume binary glTF.
    if (ext == "glb" || ext == "GLB")
        res = loader.LoadBinaryFromFile(&_model, &err, &warn, _filename.c_str());

    // assume ascii glTF.
    else
        res = loader.LoadASCIIFromFile(&_model, &err, &warn, _filename.c_str());

    if (!warn.empty())
        std::cout << "Warn: " << warn.c_str() << std::endl;

    if (!err.empty())
        std::cout << "ERR: " << err.c_str() << std::endl;

    return res;
}

DrawMode extractMode(const tinygltf::Primitive& _primitive) {
    if (_primitive.mode == TINYGLTF_MODE_TRIANGLES)
      return TRIANGLES;
    else if (_primitive.mode == TINYGLTF_MODE_TRIANGLE_STRIP)
      return TRIANGLE_STRIP;
    else if (_primitive.mode == TINYGLTF_MODE_TRIANGLE_FAN)
      return TRIANGLE_FAN;
    else if (_primitive.mode == TINYGLTF_MODE_POINTS)
      return POINTS;
    else if (_primitive.mode == TINYGLTF_MODE_LINE)
      return LINES;
    else if (_primitive.mode == TINYGLTF_MODE_LINE_LOOP)
      return LINE_LOOP;
    
    return TRIANGLES;
}

void extractIndices(const tinygltf::Model& _model, const tinygltf::Accessor& _indexAccessor, Mesh& _mesh) {
    const tinygltf::BufferView &buffer_view = _model.bufferViews[_indexAccessor.bufferView];
    const tinygltf::Buffer &buffer = _model.buffers[buffer_view.buffer];
    const uint8_t* base = &buffer.data.at(buffer_view.byteOffset + _indexAccessor.byteOffset);

    switch (_indexAccessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
            const uint32_t *p = (uint32_t*) base;
            for (size_t i = 0; i < _indexAccessor.count; ++i) {
                _mesh.addIndex( p[i] );
            }
        }; break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
            const uint16_t *p = (uint16_t*) base;
            for (size_t i = 0; i < _indexAccessor.count; ++i) {
                _mesh.addIndex( p[i] );
            }
        }; break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
            const uint8_t *p = (uint8_t*) base;
            for (size_t i = 0; i < _indexAccessor.count; ++i) {
                _mesh.addIndex( p[i] );
            }
        }; break;
    }
}

void extractVertexData(uint32_t v_pos, const uint8_t *base, int accesor_componentType, int accesor_type, bool accesor_normalized, uint32_t byteStride, float *output, uint8_t max_num_comp) {
    float v[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    uint32_t ncomp = 1;
    switch (accesor_type) {
        case TINYGLTF_TYPE_SCALAR: ncomp = 1; break;
        case TINYGLTF_TYPE_VEC2:   ncomp = 2; break;
        case TINYGLTF_TYPE_VEC3:   ncomp = 3; break;
        case TINYGLTF_TYPE_VEC4:   ncomp = 4; break;
        default:
            assert(!"invalid type");
    }
    switch (accesor_componentType) {
        case TINYGLTF_COMPONENT_TYPE_FLOAT: {
            const float *data = (float*)(base+byteStride*v_pos);
            for (uint32_t i = 0; (i < ncomp); ++i) {
                v[i] = data[i];
            }
        }
        // TODO SUPPORT OTHER FORMATS
        break;
        default:
            assert(!"Conversion Type from float to -> ??? not implemented yet");
            break;
    }
    for (uint32_t i = 0; i < max_num_comp; ++i) {
        output[i] = v[i];
    }
}

Material* extractMaterial(const tinygltf::Model& _model, const tinygltf::Material& _material, Scene* _scene, bool _verbose) {
    std::string mat_name = toLower( toUnderscore( purifyString( _material.name ) ) );

    if (_scene->materials.find(mat_name) != _scene->materials.end())
        return _scene->materials[mat_name];
        
    int texCounter = 0;
    Material* mat = new Material(mat_name);

    mat->addDefine("MATERIAL_NAME_" + toUpper(mat->name) );
    mat->addDefine("MATERIAL_BASECOLOR", (double*)_material.pbrMetallicRoughness.baseColorFactor.data(), 4);
    if (_material.pbrMetallicRoughness.baseColorTexture.index >= 0) {
        const tinygltf::Texture &tex = _model.textures[_material.pbrMetallicRoughness.baseColorTexture.index];
        const tinygltf::Image &image = _model.images[tex.source];
        std::string name = image.name + image.uri;
        if (name.empty())
            name = mat_name + toString(texCounter++);
        name = getUniformName(name);

        if (_verbose)
            std::cout << "Loading " << mat_name << " BASECOLORMAP as " << name << std::endl;

        Image* img = new Image(&image.image.at(0), image.width, image.height, image.component);
        img->name = name;
        mat->set("diffuse", img);

        if (_scene->textures.find(name) == _scene->textures.end())
            _scene->textures[name] = new Texture(img);

        mat->addDefine("MATERIAL_BASECOLORMAP", name);
    } 
    else {
        std::vector<double> c = _material.pbrMetallicRoughness.baseColorFactor;
        mat->set("diffuse", glm::vec4(float(c[0]), float(c[1]), float(c[2]), float(c[3])));
    }

    mat->addDefine("MATERIAL_EMISSIVE", (double*)_material.emissiveFactor.data(), 3);
    if (_material.emissiveTexture.index >= 0) {
        const tinygltf::Image &image = _model.images[_model.textures[_material.emissiveTexture.index].source];
        std::string name = image.name + image.uri;
        if (name.empty())
            name = mat_name + toString(texCounter++);
        name = getUniformName(name);

        Image* img = new Image(&image.image.at(0), image.width, image.height, image.component);
        img->name = name;
        mat->set("emissive", img);

        if (_verbose)
            std::cout << "Loading " << name << "for EMISSIVEMAP as " << name << std::endl;

        if (_scene->textures.find(name) == _scene->textures.end())
            _scene->textures[name] = new Texture(img);
        
        mat->addDefine("MATERIAL_EMISSIVEMAP", name);
    }
    else {
        std::vector<double> c = _material.emissiveFactor;
        mat->set("emissive", glm::vec3(float(c[0]), float(c[1]), float(c[2])));
    }

    bool isOcclusionRoughnessMetallic = false;
    mat->addDefine("MATERIAL_ROUGHNESS", _material.pbrMetallicRoughness.roughnessFactor);
    mat->addDefine("MATERIAL_METALLIC", _material.pbrMetallicRoughness.metallicFactor);
    if (_material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0) {
        tinygltf::Texture tex = _model.textures[_material.pbrMetallicRoughness.metallicRoughnessTexture.index];
        const tinygltf::Image &image = _model.images[tex.source];
        std::string name = image.name + image.uri;
        if (name.empty())
            name = mat_name + toString(texCounter++);
        name = getUniformName(name);

        if (_verbose)
            std::cout << "Loading " << name << "for METALLICROUGHNESSMAP as " << name << std::endl;

        if (_scene->textures.find(name) == _scene->textures.end()) {
            Texture* texture = new Texture();
            texture->load(image.width, image.height, image.component, image.bits, &image.image.at(0));
            _scene->textures[name] = texture;
        }

        if (_material.occlusionTexture.index >= 0) {
            const tinygltf::Image &occlussionImage = _model.images[_model.textures[_material.occlusionTexture.index].source];
            if (image.uri != "" && image.uri == occlussionImage.uri)
                isOcclusionRoughnessMetallic = true;
        }

        if (isOcclusionRoughnessMetallic) {
            mat->addDefine("MATERIAL_OCCLUSIONROUGHNESSMETALLICMAP", name);
            if (_material.occlusionTexture.strength != 1.0)
                mat->addDefine("MATERIAL_OCCLUSIONMAP_STRENGTH", _material.occlusionTexture.strength);
        }
        else
            mat->addDefine("MATERIAL_ROUGHNESSMETALLICMAP", name);
    }

     // OCCLUSION
    if (!isOcclusionRoughnessMetallic && _material.occlusionTexture.index >= 0) {
        const tinygltf::Image &image = _model.images[_model.textures[_material.occlusionTexture.index].source];
        std::string name = image.name + image.uri;
        if (name.empty())
            name = mat_name + toString(texCounter++);
        name = getUniformName(name);

        if (_verbose)
            std::cout << "Loading " << name << "for OCCLUSIONMAP as " << name << std::endl;

        if (_scene->textures.find(name) == _scene->textures.end()) {
            Texture* texture = new Texture();
            texture->load(image.width, image.height, image.component, image.bits, &image.image.at(0));
            _scene->textures[name] = texture;
        }
        mat->addDefine("MATERIAL_OCCLUSIONMAP", name);

        if (_material.occlusionTexture.strength != 1.0)
            mat->addDefine("MATERIAL_OCCLUSIONMAP_STRENGTH", _material.occlusionTexture.strength);
    }

    // NORMALMAP
    if (_material.normalTexture.index >= 0) {
        const tinygltf::Image &image = _model.images[_model.textures[_material.normalTexture.index].source];
        std::string name = image.name + image.uri;
        if (name.empty())
            name = mat_name + toString(texCounter++);
        name = getUniformName(name);

        if (_verbose)
            std::cout << "Loading " << name << "for NORMALMAP as " << name << std::endl;

        if (_scene->textures.find(name) == _scene->textures.end()) {
            Texture* texture = new Texture();
            texture->load(image.width, image.height, image.component, image.bits, &image.image.at(0));
            _scene->textures[name] = texture;
        }
        mat->addDefine("MATERIAL_NORMALMAP", name);

        if (_material.normalTexture.scale != 1.0)
            mat->addDefine("MATERIAL_NORMALMAP_SCALE", glm::vec3(_material.normalTexture.scale, _material.normalTexture.scale, 1.0));
    }

    return mat;
}

void extractMesh(const tinygltf::Model& _model, const tinygltf::Mesh& _mesh, glm::mat4 _matrix, Scene* _scene, bool _verbose, const std::string& _prefix) {
    if (_verbose)
        std::cout << "  Parsing Mesh " << _mesh.name << std::endl;

    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(_matrix)));

    for (size_t i = 0; i < _mesh.primitives.size(); ++i) {
        if (_verbose)
            std::cout << "   primitive " << i + 1 << "/" << _mesh.primitives.size() << std::endl;

        const tinygltf::Primitive &primitive = _mesh.primitives[i];

        Mesh mesh;
        if (primitive.indices >= 0)
            extractIndices(_model, _model.accessors[primitive.indices], mesh);
        mesh.setDrawMode(extractMode(primitive));

        // Extract Vertex Data
        for (auto &attrib : primitive.attributes) {
            const tinygltf::Accessor &accessor = _model.accessors[attrib.second];
            const tinygltf::BufferView &bufferView = _model.bufferViews[accessor.bufferView];
            const tinygltf::Buffer &buffer = _model.buffers[bufferView.buffer];
            int byteStride = accessor.ByteStride(bufferView);

            if (attrib.first.compare("POSITION") == 0)  {
                for (size_t v = 0; v < accessor.count; v++) {
                    glm::vec4 pos = glm::vec4(1.0);
                    extractVertexData(v, &buffer.data.at(bufferView.byteOffset + accessor.byteOffset), accessor.componentType, accessor.type, accessor.normalized, byteStride, &pos[0], 3);
                    mesh.addVertex( glm::vec3(_matrix * pos) );
                }
            }

            else if (attrib.first.compare("COLOR_0") == 0)  {
                for (size_t v = 0; v < accessor.count; v++) {
                    glm::vec4 col = glm::vec4(1.0f);
                    extractVertexData(v, &buffer.data.at(bufferView.byteOffset + accessor.byteOffset), accessor.componentType, accessor.type, accessor.normalized, byteStride, &col[0], 4);
                    mesh.addColor(col);
                }
            }

            else if (attrib.first.compare("NORMAL") == 0)  {
                for (size_t v = 0; v < accessor.count; v++) {
                    glm::vec3 nor;
                    extractVertexData(v, &buffer.data.at(bufferView.byteOffset + accessor.byteOffset), accessor.componentType, accessor.type, accessor.normalized, byteStride, &nor[0], 3);
                    mesh.addNormal( normalize(normalMatrix * nor) );
                }
            }

            else if (attrib.first.compare("TEXCOORD_0") == 0)  {
                for (size_t v = 0; v < accessor.count; v++) {
                    glm::vec2 uv;
                    extractVertexData(v, &buffer.data.at(bufferView.byteOffset + accessor.byteOffset), accessor.componentType, accessor.type, accessor.normalized, byteStride, &uv[0], 2);
                    mesh.addTexCoord(uv);
                }
            }

            else if (attrib.first.compare("TANGENT") == 0)  {
                for (size_t v = 0; v < accessor.count; v++) {
                    glm::vec4 tan;
                    extractVertexData(v, &buffer.data.at(bufferView.byteOffset + accessor.byteOffset), accessor.componentType, accessor.type, accessor.normalized, byteStride, &tan[0], 4);
                    mesh.addTangent(tan);
                }
            }

            else {
                std::cout << " " << std::endl;
                std::cout << "Attribute: " << attrib.first << std::endl;
                std::cout << "  type        :" << accessor.type << std::endl;
                std::cout << "  component   :" << accessor.componentType << std::endl;
                std::cout << "  normalize   :" << accessor.normalized << std::endl;
                std::cout << "  bufferView  :" << accessor.bufferView << std::endl;
                std::cout << "  byteOffset  :" << accessor.byteOffset << std::endl;
                std::cout << "  count       :" << accessor.count << std::endl;
                std::cout << "  byteStride  :" << byteStride << std::endl;
                std::cout << " "<< std::endl;
            }
        }

        if (_verbose) {
            std::cout << "    vertices = " << mesh.getVertices().size() << std::endl;
            std::cout << "    colors   = " << mesh.getColors().size() << std::endl;
            std::cout << "    normals  = " << mesh.getNormals().size() << std::endl;
            std::cout << "    uvs      = " << mesh.getTexCoords().size() << std::endl;
            std::cout << "    indices  = " << mesh.getIndices().size() << std::endl;

            if (mesh.getDrawMode() == GL_TRIANGLES) {
                std::cout << "    triang.  = " << mesh.getIndices().size()/3 << std::endl;
            }
            else if (mesh.getDrawMode() == GL_LINES ) {
                std::cout << "    lines    = " << mesh.getIndices().size()/2 << std::endl;
            }
        }

        if ( !mesh.haveNormals() )
            if ( mesh.computeNormals() )
                if ( _verbose )
                    std::cout << "    . Compute normals" << std::endl;

        if ( mesh.computeTangents() )
            if ( _verbose )
                std::cout << "    . Compute tangents" << std::endl;

        // A primitive without an assigned material has material index -1;
        // indexing _model.materials with it is out of bounds (crash). Fall
        // back to a shared "default" material in that case.
        Material* mat = nullptr;
        if (primitive.material >= 0 && primitive.material < (int)_model.materials.size())
            mat = extractMaterial( _model, _model.materials[primitive.material], _scene, _verbose );
        else {
            if (_scene->materials.find("default") == _scene->materials.end())
                _scene->materials["default"] = new Material("default");
            mat = _scene->materials["default"];
        }

        // Namespace the mesh by the file prefix so several glTFs can coexist.
        std::string name = _prefix.empty() ? _mesh.name : _prefix + "_" + _mesh.name;
        _scene->models[name] = new Model(name, mesh, mat);
    }
};

// Copy a glTF KHR_lights_punctual light's parameters (placed at a node's world
// transform) into a vera::Light.
void applyGltfLight(Light* _light, const tinygltf::Light& _l, const glm::mat4& _matrix) {
    // A glTF light shines down its node's local -Z; its position is the node's
    // world translation.
    _light->setPosition( glm::vec3(_matrix[3]) );
    _light->direction = glm::normalize(glm::mat3(_matrix) * glm::vec3(0.0f, 0.0f, -1.0f));

    if (_l.type == "directional")   _light->setType(LIGHT_DIRECTIONAL);
    else if (_l.type == "spot")     _light->setType(LIGHT_SPOT);
    else                            _light->setType(LIGHT_POINT); // "point"

    if (_l.color.size() == 3)
        _light->color = glm::vec3((float)_l.color[0], (float)_l.color[1], (float)_l.color[2]);

    // NOTE: glTF KHR_lights_punctual intensities are physical units (candela
    // for point/spot, lux for directional), routinely in the tens to thousands.
    // glslViewer's default shading treats u_lightIntensity as a ~1.0 multiplier,
    // so copying the raw value blows the scene out. Keep the viewer's neutral
    // default intensity instead (position/direction/color/type still come from
    // the glTF). A physically-based intensity mapping would need a calibrated
    // exposure pipeline that the default shaders don't have.
    if (_l.range > 0.0)
        _light->falloff = (float)_l.range;
}

// Bring a glTF light into the scene. The FIRST glTF light is transferred INTO
// the built-in "default" light (reusing that object) rather than replacing it,
// so all the code that references lights["default"] keeps working and it still
// maps to u_light. Subsequent lights are added as "light1", "light2", ...
// (-> u_light1, u_light2 in Uniforms::feedTo). _counter tracks how many glTF
// lights have been seen so far across the (recursive) node traversal.
void extractLight(const tinygltf::Light& _l, const glm::mat4& _matrix, Scene* _scene, bool _verbose, int& _counter) {
    Light* light = nullptr;
    std::string name;

    if (_counter == 0) {
        name = "default";
        LightsMap::iterator it = _scene->lights.find("default");
        if (it != _scene->lights.end() && it->second != nullptr)
            light = it->second;                 // reuse the built-in default
        else {
            light = new Light();
            _scene->lights["default"] = light;
        }
    }
    else {
        name = "light" + toString(_counter);
        light = new Light();
        _scene->lights[name] = light;
    }

    applyGltfLight(light, _l, _matrix);
    _scene->setHaveLights(true);                // scene provides its own lighting
    _counter++;

    if (_verbose)
        std::cout << "// glTF light '" << _l.name << "' (" << _l.type << ") added as u_"
                  << (name == "default" ? "light" : name) << std::endl;
}

// bind models
void extractNodes(const tinygltf::Model& _model, const tinygltf::Node& _node, glm::mat4 _matrix, Scene* _scene, bool _verbose, const std::string& _prefix, int& _lightCounter) {
    if (_verbose)
        std::cout << "Entering node " << _node.name << std::endl;

    glm::mat4 R = glm::mat4(1.0f);
    glm::mat4 S = glm::mat4(1.0f);
    glm::mat4 T = glm::mat4(1.0f);

    if (_node.rotation.size() == 4) {
        glm::quat q = glm::make_quat(_node.rotation.data());
        R = glm::toMat4( q );
    }

    if (_node.scale.size() == 3)
        S = glm::scale( glm::make_vec3(_node.scale.data()) );

    if (_node.translation.size() == 3)
        T = glm::translate( glm::make_vec3(_node.translation.data()) );

    glm::mat4 localMatrix = T * R * S; 

    if (_node.matrix.size() == 16)
        localMatrix = glm::make_mat4x4(_node.matrix.data());

    _matrix = _matrix * localMatrix;

    if (_node.mesh >= 0)
        extractMesh(_model, _model.meshes[ _node.mesh ], _matrix, _scene, _verbose, _prefix);

    // KHR_lights_punctual: a node points at a light via its extensions map
    // (tinygltf has no dedicated Node::light field).
    {
        tinygltf::ExtensionMap::const_iterator extIt = _node.extensions.find("KHR_lights_punctual");
        if (extIt != _node.extensions.end() && extIt->second.Has("light")) {
            int lightIdx = (int)extIt->second.Get("light").GetNumberAsInt();
            if (lightIdx >= 0 && lightIdx < (int)_model.lights.size())
                extractLight(_model.lights[lightIdx], _matrix, _scene, _verbose, _lightCounter);
        }
    }

    if (_node.camera >= 0)
        if (_verbose)
            std::cout << "  w camera" << std::endl;
        // TODO extract camera
    
    for (size_t i = 0; i < _node.children.size(); i++) {
        extractNodes(_model, _model.nodes[ _node.children[i] ], _matrix, _scene, _verbose, _prefix, _lightCounter);
    }
};

bool loadGLTF( const std::string& _filename, Scene* _scene, bool _verbose, const std::string& _prefix) {
    tinygltf::Model model;

    if ( !loadModel(_filename, model) ) {
        std::cout << "Failed to load .glTF : " << _filename << std::endl;
        return false;
    }

    // Shared across the whole (recursive) node traversal so the first glTF light
    // becomes the "default"/u_light and the rest become u_light1, u_light2, ...
    int lightCounter = 0;

    const tinygltf::Scene &scene = model.scenes[model.defaultScene];
    for (size_t i = 0; i < scene.nodes.size(); ++i)
        extractNodes(model, model.nodes[scene.nodes[i]], glm::mat4(1.0), _scene, _verbose, _prefix, lightCounter);

    return true;
}

}
