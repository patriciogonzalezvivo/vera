#pragma once

#include <map>
#include <vector>
#include <string>

#include "gl/shader.h"

#include "gl/texture.h"
#include "gl/textureStream.h"
#include "gl/textureStreamAudio.h"

#include "gl/fbo.h"
#include "gl/pingpong.h"
#include "gl/pyramid.h"

#include "scene/light.h"
#include "scene/camera.h"
#include "scene/model.h"
#include "scene/material.h"
#include "scene/label.h"

namespace vera {

// Textures
typedef std::map<std::string, Texture*>         TexturesMap;
typedef std::map<std::string, TextureStream*>   TextureStreamsMap;
typedef std::map<std::string, TextureCube*>     TextureCubesMap;

// Buffers
typedef std::vector<Fbo>                        BuffersList;
typedef std::vector<PingPong>                   DoubleBuffersList;
typedef std::vector<Pyramid>                    PyramidsList;

// 3D Scene
typedef std::map<std::string, Light*>           LightsMap;
typedef std::map<std::string, Camera*>          CamerasMap;
typedef std::map<std::string, Model*>           ModelsMap;
typedef std::map<std::string, Material>         MaterialsMap;
typedef std::vector<Label*>                     LabelsList;

class Scene {
public:
    Scene();
    virtual ~Scene();

    // Load, update & clear scene 
    virtual void        load(const std::string& _name, bool _verbose = false);
    virtual void        update();
    virtual void        clear();
    
    virtual void        flagChange();
    virtual void        unflagChange();
    virtual bool        haveChange() const;

    // Feed scene uniforms to shader 
    virtual bool        feedTo(Shader &_shader, bool _lights = true, bool _buffers = true);
    virtual bool        feedTo(Shader *_shader, bool _lights = true, bool _buffers = true);

    // defines
    virtual void        addDefine(const std::string& _define, const std::string& _value);
    virtual void        delDefine(const std::string& _define);
    virtual void        printDefines();

    // Buffers
    BuffersList         buffers;
    DoubleBuffersList   doubleBuffers;
    PyramidsList        pyramids;
    virtual void        printBuffers();
    virtual void        clearBuffers();

    // Textures
    TexturesMap         textures;
    virtual bool        haveTexture(const std::string& _name ) const;
    virtual void        setTexture(const std::string& _name, Texture* _texture);
    virtual bool        addTexture(const std::string& _name, const std::string& _path, bool _flip = true, bool _verbose = true);
    virtual bool        addBumpTexture(const std::string& _name, const std::string& _path, bool _flip = true, bool _verbose = true);
    virtual Texture*    getTexture(const std::string& _name);
    virtual void        printTextures();
    virtual void        clearTextures();

    // Streams (videos, camera, image sequences, audio textures, etc)
    TextureStreamsMap   streams;
    virtual bool        haveStreamingTexture(const std::string& _name) const;
    virtual void        setStreamingTexture(const std::string& _name, TextureStream* _stream);
    virtual bool        addStreamingTexture(const std::string& _name, const std::string& _url, bool _flip = true, bool _device = false, bool _verbose = true);
    virtual bool        addStreamingAudioTexture(const std::string& _name, const std::string& device_id, bool _flip = false, bool _verbose = true);
    virtual TextureStream* getStreamingTexture(const std::string& _name);
    virtual void        printStreams();

    virtual void        setStreamPlay(const std::string& _name);
    virtual void        setStreamStop(const std::string& _name);
    virtual void        setStreamRestart(const std::string& _name);
    virtual float       getStreamTime(const std::string& _name);
    virtual void        setStreamTime(const std::string& _name, float _time);
    virtual float       getStreamPct(const std::string& _name);
    virtual void        setStreamPct(const std::string& _name, float _pct);
    virtual float       getStreamSpeed(const std::string& _name);
    virtual void        setStreamSpeed(const std::string& _name, float _speed);

    virtual void        setStreamsPlay();
    virtual void        setStreamsStop();
    virtual void        setStreamsRestart();
    
    virtual void        setStreamsPct(float _pct);
    virtual void        setStreamsTime(float _time);
    virtual void        setStreamsSpeed(float _speed);
    virtual void        setStreamsPrevs(size_t _total);

    virtual void        clearStreams();

    // Cubemap
    TextureCubesMap     cubemaps;
    virtual bool        haveCubemap(const std::string& _name ) const;
    virtual void        setCubemap(const std::string& _name, TextureCube* _cubemap);
    virtual bool        addCubemap(const std::string& _name, const std::string& _filename, bool _verbose = true);
    virtual TextureCube* getCubemap(const std::string& _name);
    virtual void        setActiveCubemap(const std::string& _name);
    virtual TextureCube* getActiveCubemap() const { return m_activeCubemap; }
    virtual void        clearCubemaps();
    virtual void        printCubemaps();

    // Skybox
    virtual void        setSunPosition(const glm::vec3& _v);
    virtual void        setSunPosition(float _az, float _elev);
    virtual void        setSkyTurbidity(float _turbidity);
    virtual void        setGroundAlbedo(const glm::vec3& _albedo);
    virtual float       getSunAzimuth() const;
    virtual float       getSunElevation() const;
    virtual float       getSkyTurbidity() const;
    virtual glm::vec3   getGroundAlbedo() const;

    // Camera
    CamerasMap          cameras;
    virtual bool        haveCamera(const std::string& _name ) const;
    virtual void        setCamera(const std::string& _name, Camera* _camera);
    virtual void        setActiveCamera(const std::string& _name);
    virtual Camera*     getActiveCamera() const { return m_activeCamera; }
    virtual Camera*     getCamera(const std::string& _name);
    virtual void        printCameras();
    virtual void        clearCameras();

    // Lights
    LightsMap           lights;
    virtual bool        haveLight(const std::string& _name) const;
    virtual void        setLight(const std::string& _name, Light* _light);
    virtual Light*      getLight(const std::string& _name);
    virtual void        enableLights(bool _v) { m_enableLights = true; }
    virtual bool        getLightsEnabled() const { return m_enableLights; }
    virtual void        printLights();
    virtual void        clearLights();

    // Materials
    MaterialsMap        materials;
    virtual bool        haveMaterial(const std::string& _name) const;
    virtual void        setMaterial(const std::string& _name, Material& _material);
    virtual void        printMaterials();
    virtual void        clearMaterials();

    // Labels
    LabelsList          labels;
    virtual void        printLabels();
    virtual void        clearLabels();

    // Models
    ModelsMap           models;
    virtual bool        haveModel(const std::string& _name) const;
    virtual void        setModel(const std::string& _name, Model* _model);
    virtual Model*      getModel(const std::string& _name);
    virtual void        printModels();
    virtual void        clearModels();

    // Node Tree
    std::vector<Node*>  root;

protected:
    SkyData             m_skybox;

    size_t              m_streamsPrevs;
    bool                m_streamsPrevsChange;

    Camera*             m_activeCamera;
    TextureCube*        m_activeCubemap;

    bool                m_enableLights;
    bool                m_change;
};

}