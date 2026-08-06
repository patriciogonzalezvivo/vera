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

class Scene {
public:
    Scene();
    virtual ~Scene();

    // Load, update & clear scene
    // _prefix (optional) namespaces every model key/name produced by this file
    // so multiple geometry files can coexist without clobbering each other and
    // can be hot-reloaded independently (see removeModelsByPrefix).
    virtual void        load(const std::string& _name, bool _verbose = false, const std::string& _prefix = "");
    virtual void        update();
    virtual void        clear();

    // Change state
    virtual void        flagChange();
    virtual void        resetChange();
    virtual bool        haveChange();

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
    virtual void        setStreamFrame(const std::string& _name, size_t _frame);


    virtual void        setStreamsPlay();
    virtual void        setStreamsStop();
    virtual void        setStreamsRestart();
    
    virtual void        setStreamsPct(float _pct);
    virtual void        setStreamsTime(float _time);
    virtual void        setStreamsSpeed(float _speed);
    virtual void        setStreamsPrevs(size_t _total);
    virtual void        setStreamsFrame(size_t _frame);

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
    // True when the loaded scene defines its own lights (e.g. a glTF with
    // KHR_lights_punctual). The viewer uses this to avoid overriding them with
    // its default sun placement.
    bool                haveLights() const { return m_haveLights; }
    void                setHaveLights(bool _v) { m_haveLights = _v; }

    // Materials
    MaterialsMap        materials;
    virtual void        printMaterials();
    virtual void        clearMaterials();

    // Models
    ModelsMap           models;
    virtual void        printModels();
    virtual void        clearModels();
    // Remove (and delete) every model whose key belongs to a given file prefix,
    // i.e. key == _prefix or key starts with (_prefix + "_"). Used to hot-reload
    // a single geometry file without disturbing the others.
    virtual void        removeModelsByPrefix(const std::string& _prefix);

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
    LabelSettings       labelSettings;
    virtual void        printLabels();
    virtual void        clearLabels();

    TextureCube*        activeCubemap;
    Camera*             activeCamera;
    Camera*             lastCamera;
    Font*               activeFont;

protected:
    SkyData             m_skybox;
    size_t              m_skyboxSize;
    bool                m_skyboxFlip;

    size_t              m_streamsPrevs;
    bool                m_streamsPrevsChange;

    bool                m_changed;
    bool                m_haveLights = false;

};

}