#pragma once

#include <map>
#include <vector>
#include <string>

#include "../gl/shader.h"

#include "../gl/texture.h"
#include "../gl/textureStream.h"
#include "../gl/textureStreamAudio.h"

#include "../gl/fbo.h"
#include "../gl/pingpong.h"
#include "../gl/pyramid.h"

#include "image.h"
#include "light.h"
#include "camera.h"
#include "model.h"
#include "material.h"
#include "label.h"

namespace vera {

// Textures
typedef std::map<std::string, Texture*>         TexturesMap;
typedef std::map<std::string, TextureStream*>   TextureStreamsMap;
typedef std::map<std::string, TextureCube*>     TextureCubesMap;

// 3D Scene
typedef std::map<std::string, Light*>           LightsMap;
typedef std::map<std::string, Camera*>          CamerasMap;
typedef std::map<std::string, Model*>           ModelsMap;
typedef std::map<std::string, Material*>        MaterialsMap;
typedef std::map<std::string, Shader*>          ShadersMap;

typedef std::map<std::string, Font*>            FontsMap;
typedef std::vector<Label*>                     LabelsList;

class Scene {
public:
    Scene();
    virtual ~Scene();

    // Load, update & clear scene 
    virtual void        load(const std::string& _name, bool _verbose = false);
    virtual void        update();
    virtual void        clear();

    // Textures
    TexturesMap         textures;
    virtual bool        addTexture(const std::string& _name, const std::string& _path, bool _flip = true, bool _verbose = true);
    virtual bool        addTexture(const std::string& _name, const Image& _image, bool _flip = true, bool _verbose = true);
    virtual bool        addBumpTexture(const std::string& _name, const std::string& _path, bool _flip = true, bool _verbose = true);
    virtual void        printTextures();
    virtual void        clearTextures();

    // Streams (videos, camera, image sequences, audio textures, etc)
    TextureStreamsMap   streams;
    virtual bool        addStreamingTexture(const std::string& _name, const std::string& _url, bool _flip = true, bool _device = false, bool _verbose = true);
    virtual bool        addStreamingAudioTexture(const std::string& _name, const std::string& device_id, bool _flip = false, bool _verbose = true);
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

    // Cubemap
    TextureCubesMap     cubemaps;
    virtual bool        addCubemap(const std::string& _name, const std::string& _filename, bool _verbose = true);
    virtual void        clearCubemaps();
    virtual void        printCubemaps();
    virtual void        printCubemapSH();

    // Skybox
    virtual void        setSunPosition(const glm::vec3& _v);
    virtual void        setSunPosition(float _az, float _elev, float _distance = 10000.0f);
    virtual void        setSkyTurbidity(float _turbidity);
    virtual void        setSkyFlip(bool _flip) { m_skyboxFlip = _flip; }
    virtual void        setSkySize(size_t _size) { m_skyboxSize = _size; }
    virtual void        setGroundAlbedo(const glm::vec3& _albedo);
    virtual float       getSunAzimuth() const;
    virtual float       getSunElevation() const;
    virtual float       getSkyTurbidity() const;
    virtual glm::vec3   getGroundAlbedo() const;

    // Camera
    CamerasMap          cameras;
    virtual void        printCameras();
    virtual void        clearCameras();

    // Lights
    LightsMap           lights;
    virtual void        printLights();
    virtual void        clearLights();

    // Materials
    MaterialsMap        materials;
    virtual void        printMaterials();
    virtual void        clearMaterials();

    // Models
    ModelsMap           models;
    virtual void        printModels();
    virtual void        clearModels();

    // Node Tree
    std::vector<Node*>  root;

    ShadersMap          shaders;
    virtual void        printShaders();
    virtual void        clearShaders();

    // Fonts
    FontsMap            fonts;
    virtual bool        addFont(const std::string& _name, const std::string& _path);
    virtual Font*       getDefaultFont();
    virtual void        printFonts();
    virtual void        clearFonts();

    // Labels
    LabelsList          labels;
    virtual void        printLabels();
    virtual void        clearLabels();

    TextureCube*        activeCubemap;
    Camera*             activeCamera;
    Font*               activeFont;

protected:
    SkyData             m_skybox;
    size_t              m_skyboxSize;
    bool                m_skyboxFlip;

    size_t              m_streamsPrevs;
    bool                m_streamsPrevsChange;

};

}