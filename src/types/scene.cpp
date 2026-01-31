#include "vera/types/scene.h"

#include <sys/stat.h>

#include "vera/ops/fs.h"
#include "vera/ops/pixel.h"
#include "vera/ops/string.h"

#include "vera/io/ply.h"
#include "vera/io/obj.h"
#include "vera/io/gltf.h"
#include "vera/io/stl.h"

#include "vera/gl/textureBump.h"
#include "vera/gl/textureStreamSequence.h"

#if defined(SUPPORT_MMAL)
#include "vera/gl/textureStreamMMAL.h"
#endif

#if defined(SUPPORT_LIBAV) 
#include "vera/gl/textureStreamAV.h"
#endif

#if defined(DRIVER_BROADCOM) && defined(SUPPORT_OMAX)
#include "vera/gl/textureStreamOMX.h"
#endif

namespace vera {

Scene::Scene(): 
    activeCubemap(nullptr), 
    activeCamera(nullptr),
    activeFont(nullptr),
    m_changed(false),
    m_skyboxSize(1024),
    m_skyboxFlip(false),
    m_streamsPrevs(0), 
    m_streamsPrevsChange(false) {
    
    // Default Active Camera
    activeCamera = new Camera();
    cameras["default"] = activeCamera;

    // Default Skybox Texture Cube
    activeCubemap = new TextureCube();
    cubemaps["default"] = activeCubemap;

    // Defualt light source
    lights["default"] = new Light(glm::vec3(0.0, 1000.0, 1000.0), -1.0);
}

Scene::~Scene() {
    clear();
}

void Scene::load(const std::string& _filename, bool _verbose) {
    std::string ext = vera::getExt(_filename);

    // If the geometry is a PLY it's easy because is only one mesh
    if ( ext == "ply" || ext == "PLY" )
        loadPLY( _filename, this, _verbose);

    // If it's a OBJ could be more complicated because they can contain several meshes and materials
    else if ( ext == "obj" || ext == "OBJ" )
        loadOBJ( _filename, this, _verbose);

    // If it's a GLTF it's not just multiple meshes and materials but also nodes, lights and cameras
    else if ( ext == "glb" || ext == "GLB" || ext == "gltf" || ext == "GLTF" )
        loadGLTF( _filename, this, _verbose);

    // If it's a STL 
    else if ( ext == "stl" || ext == "STL" )
        loadSTL( _filename, this, _verbose);

    else if ( ext == "splat" || ext == "SPLAT" ) {
        // load SPLAT as a Gsplat model
        Gsplat* gsplat = new Gsplat();

        if (gsplat->load(_filename)) {
            Model* model = new Model( _filename, gsplat );
            // rotate to put Z as up axis
            // model->rotate( glm::quat( glm::radians(-90.0f), glm::vec3(1.0f,0.0f,0.0f) ) );
            model->scale( glm::vec3(100.0f) );
            // scale up the model x10 times
            models[ model->getName() ] = model;
            if (_verbose) {
                std::cout << "// " << _filename << " loaded as Gsplat model: " << model->getName() << std::endl;
            }
        } else {
            delete gsplat;
        }
    }

    m_changed = true;
}

void Scene::update() {
    if (m_skybox.change) {
        cubemaps["default"]->load(&m_skybox, m_skyboxSize, m_skyboxFlip);
        m_skybox.change = false;
    }

    if (m_streamsPrevsChange) {
        m_streamsPrevsChange = false;
        for (TextureStreamsMap::iterator it = streams.begin(); it != streams.end(); it++)
            it->second->setPrevTextures(m_streamsPrevs);
    }

    for (TextureStreamsMap::iterator it = streams.begin(); it != streams.end(); ++it)
        it->second->update();

    m_changed = false;
}

void Scene::clear() {
    clearTextures();

    clearCubemaps();
    
    clearLights();
    clearCameras();
    
    clearModels();
    clearMaterials();
    clearShaders();
    
    clearLabels();
}


void Scene::flagChange() {
    m_changed = true;

    if (activeCamera)
        activeCamera->bChange = true;
}

void Scene::resetChange() {
    m_changed = false;

    for (vera::LightsMap::iterator it = lights.begin(); it != lights.end(); ++it)
        it->second->bChange = false;

    if (activeCamera)
        activeCamera->bChange = false;
}

bool Scene::haveChange() { 
    if (activeCamera)
        if (activeCamera->bChange)
            return true;
            
    for (vera::LightsMap::const_iterator it = lights.begin(); it != lights.end(); ++it)
        if (it->second->bChange)
            return true;

    if (m_changed || streams.size() > 0)
        return true;

    return false;
}

// TEXTURES
//
bool Scene::addTexture(const std::string& _name, const std::string& _path, bool _flip, bool _verbose) {
    if (textures.find(_name) == textures.end()) {
        struct stat st;

        // If we can not get file stamp proably is not a file
        if (stat(_path.c_str(), &st) != 0 )
            std::cerr << "Error watching for file " << _path << std::endl;

        // If we can lets proceed creating a texgure
        else {

            Texture* tex = new Texture();
            // load an image into the texture
            if (tex->load(_path, _flip)) {
                
                // the image is loaded finish add the texture to the uniform list
                textures[_name] = tex;

                if (_verbose) {
                    std::cout << "// " << _path << " loaded as: " << std::endl;
                    std::cout << "uniform sampler2D   " << _name  << ";"<< std::endl;
                    std::cout << "uniform vec2        " << _name  << "Resolution;"<< std::endl;
                }

                if (haveExt(_path, "jpeg")) {
                    int width, height;
                    unsigned char* pixels = loadPixelsDepth(_path, &width, &height, _flip);
                    if (pixels) {
                        Texture* tex_dm = new Texture();
                        if (tex_dm->load(width, height, 3, 8, pixels)) {
                            textures[ _name + "Depth"] = tex_dm;
                            if (_verbose) {
                                std::cout << "uniform sampler2D   " << _name  << "Depth;"<< std::endl;
                                std::cout << "uniform vec2        " << _name  << "DepthResolution;"<< std::endl;
                            }
                        }   
                        freePixels(pixels);
                    }
                }

                m_changed = true;
                return true;
            }
            else
                delete tex;
        }
    }
    return false;
}

bool Scene::addTexture(const std::string& _name, const Image& _image, bool _flip, bool _verbose) {
    if (textures.find(_name) == textures.end()) {
        Texture* tex = new Texture();

        // TODO:
        //  - flip

        // load an image into the texture
        if (tex->load(_image)) {
            
            // the image is loaded finish add the texture to the uniform list
            textures[_name] = tex;
            
            if (_verbose) {
                std::cout << "uniform sampler2D   " << _name  << ";"<< std::endl;
                std::cout << "uniform vec2        " << _name  << "Resolution;"<< std::endl;
            }

            m_changed = true;

            return true;
        }
        else
            delete tex;
    }
    else {
        // TODO:
        //  - flip
        //  - error handling

        textures[_name]->load(_image);
        Texture* tex = new Texture();
    }
    return false;
}

bool Scene::addBumpTexture(const std::string& _name, const std::string& _path, bool _flip, bool _verbose) {
    if (textures.find(_name) == textures.end()) {
        struct stat st;

        // If we can not get file stamp proably is not a file
        if (stat(_path.c_str(), &st) != 0 )
            std::cerr << "Error watching for file " << _path << std::endl;
        
        // If we can lets proceed creating a texgure
        else {
            TextureBump* tex = new TextureBump();

            // load an image into the texture
            if (tex->load(_path, _flip)) {

                // the image is loaded finish add the texture to the uniform list
                textures[_name] = (Texture*)tex;

                if (_verbose) {
                    std::cout << "// " << _path << " loaded and transform to normalmap as: " << std::endl;
                    std::cout << "uniform sampler2D   " << _name  << ";"<< std::endl;
                    std::cout << "uniform vec2        " << _name  << "Resolution;"<< std::endl;
                }

                m_changed = true;
                return true;
            }
            else
                delete tex;
        }
    }
    return false;
}


void Scene::printTextures() {
    for (TexturesMap::iterator it = textures.begin(); it != textures.end(); ++it) {
        std::cout << "uniform sampler2D " << it->first << "; // " << it->second->getFilePath() << std::endl;
        std::cout << "uniform vec2 " << it->first << "Resolution; // " << toString(it->second->getWidth(), 1) << "," << toString(it->second->getHeight(), 1) << std::endl;
    }
}

void Scene::clearTextures() {
    for (TexturesMap::iterator it = textures.begin(); it != textures.end(); ++it)
        delete (it->second);
    textures.clear();
    streams.clear();
    m_changed = true;
}

bool Scene::addStreamingTexture( const std::string& _name, const std::string& _url, bool _vflip, bool _device, bool _verbose) {
    if (textures.find(_name) == textures.end()) {

        // Check if it's an folder or a wildcard path
        if ( haveWildcard(_url) || isFolder(_url) ) {
            TextureStreamSequence *tex = new TextureStreamSequence();

            std::string url = _url;
            if (isFolder(_url)) {
                url = _url + "/*";
            }
            if (tex->load(url, _vflip)) {
                // the image is loaded finish add the texture to the uniform list
                textures[_name] = (Texture*)tex;
                streams[_name] = (TextureStream*)tex;

                if (_verbose) {
                    std::cout << "// " << _url << " sequence loaded as streaming texture: " << std::endl;
                    std::cout << "uniform sampler2D   " << _name  << ";"<< std::endl;
                    std::cout << "uniform sampler2D   " << _name  << "Prev[STREAMS_PREVS];"<< std::endl;
                    std::cout << "uniform vec2        " << _name  << "Resolution;"<< std::endl;
                    std::cout << "uniform float       " << _name  << "Time;" << std::endl;
                    std::cout << "uniform float       " << _name  << "Duration;" << std::endl;
                    std::cout << "uniform float       " << _name  << "CurrentFrame;"<< std::endl;
                    std::cout << "uniform float       " << _name  << "TotalFrames;"<< std::endl;
                    std::cout << "uniform float       " << _name  << "Fps;"<< std::endl;
                }

                m_changed = true;

                return true;
            }
            else
                delete tex;
            
        }
#if defined(SUPPORT_MMAL)
        // if the user is asking for a device on a RaspberryPI hardware
        else if (_device) {
            TextureStreamMMAL* tex = new TextureStreamMMAL();

            // load an image into the texture
            if (tex->load(_url, _vflip)) {
                // the image is loaded finish add the texture to the uniform list
                textures[_name] = (Texture*)tex;
                streams[_name] = (TextureStream*)tex;

                if (_verbose) {
                    std::cout << "// " << _url << " loaded as streaming texture: " << std::endl;
                    std::cout << "uniform sampler2D   " << _name  << ";"<< std::endl;
                    std::cout << "uniform vec2        " << _name  << "Resolution;"<< std::endl;
                }

                m_changed = true;
                return true;
            }
            else
                delete tex;
        }
#endif
#if defined(DRIVER_BROADCOM) && defined(SUPPORT_OMAX)
        else if ( haveExt(_url,"h264") || haveExt(_url,"H264") ) {
            TextureStreamOMX* tex = new TextureStreamOMX();

            // load an image into the texture
            if (tex->load(_url, _vflip)) {
                // the image is loaded finish add the texture to the uniform list
                textures[_name] = (Texture*)tex;
                streams[_name] = (TextureStream*)tex;

                if (_verbose) {
                    std::cout << "// " << _url << " loaded as streaming texture: " << std::endl;
                    std::cout << "uniform sampler2D   " << _name  << ";"<< std::endl;
                    std::cout << "uniform vec2        " << _name  << "Resolution;"<< std::endl;
                }

                m_changed = true;
                return true;
            }
            else
                delete tex;
        }
#endif
        else {
#if defined(SUPPORT_LIBAV)
        TextureStreamAV* tex = new TextureStreamAV(_device);

        // load an image into the texture
        if (tex->load(_url, _vflip)) {
            // the image is loaded finish add the texture to the uniform list
            textures[_name] = (Texture*)tex;
            streams[_name] = (TextureStream*)tex;

            if (_verbose) {
                std::cout << "// " << _url << " loaded as streaming texture: " << std::endl;
                std::cout << "uniform sampler2D   " << _name  << ";"<< std::endl;
                std::cout << "uniform sampler2D   " << _name  << "Prev[STREAMS_PREVS];"<< std::endl;
                std::cout << "uniform vec2        " << _name  << "Resolution;"<< std::endl;

                if (!_device) {
                    std::cout << "uniform float       " << _name  << "Time;" << std::endl;
                    std::cout << "uniform float       " << _name  << "Duration;" << std::endl;
                    std::cout << "uniform float       " << _name  << "CurrentFrame;" << std::endl;
                    std::cout << "uniform float       " << _name  << "TotalFrames;" << std::endl;
                    std::cout << "uniform float       " << _name  << "Fps;" << std::endl;
                }
            }

            m_changed = true;
            return true;
        }
        else
            delete tex;
#endif
        }


    }
    return false;
}

bool Scene::addStreamingAudioTexture(const std::string& _name, const std::string& device_id, bool _flip, bool _verbose) {
#if defined(SUPPORT_LIBAV)
    TextureStreamAudio *tex = new TextureStreamAudio();

    // TODO: add flipping mode for audio texture
    if (tex->load(device_id, _flip)) {
        // the image is loaded finish add the texture to the uniform list
        textures[_name] = (Texture*)tex;
        streams[_name] = (TextureStream*)tex;

        if (_verbose) {
            std::cout << "loaded audio texture: " << std::endl;
            std::cout << "uniform sampler2D   " << _name  << ";"<< std::endl;
            std::cout << "uniform vec2        " << _name  << "Resolution;"<< std::endl;
        }

        m_changed = true;
        return true;
    }
    else
#endif
        return false;
}

void Scene::setStreamPlay( const std::string& _name) {
    TextureStreamsMap::iterator it = streams.find(_name);
    if (it != streams.end()) {
        it->second->play();
        m_changed = true;

    }
}

void Scene::setStreamFrame( const std::string& _name, size_t _frame) {
    TextureStreamsMap::iterator it = streams.find(_name);
    if (it != streams.end()) {
        it->second->setFrame(_frame);
        m_changed = true;
    }
}

void Scene::setStreamStop( const std::string& _name) {
    TextureStreamsMap::iterator it = streams.find(_name);
    if (it != streams.end()) {
        it->second->stop();
        m_changed = true;
    }
}

void Scene::setStreamRestart( const std::string& _name ) {
    TextureStreamsMap::iterator it = streams.find(_name);
    if (it != streams.end()) {
        it->second->restart();
        m_changed = true;
    }
}

float Scene::getStreamTime( const std::string& _name) {
    TextureStreamsMap::iterator it = streams.find(_name);
    if (it != streams.end()) {}
        return it->second->getTime();
    return 0.0f;
}

float Scene::getStreamSpeed( const std::string& _name) {
    TextureStreamsMap::iterator it = streams.find(_name);
    if (it != streams.end())
        return it->second->getSpeed();
    return 0.0f;
}

void Scene::setStreamSpeed( const std::string& _name, float _speed ) {
    TextureStreamsMap::iterator it = streams.find(_name);
    if (it != streams.end()) {
        it->second->setSpeed(_speed);
        m_changed = true;
    }
}

float Scene::getStreamPct( const std::string& _name) {
    TextureStreamsMap::iterator it = streams.find(_name);
    if (it != streams.end())
        return it->second->getPct();
    return 0.0f;
}

void Scene::setStreamPct( const std::string& _name, float _pct ) {
    TextureStreamsMap::iterator it = streams.find(_name);
    if (it != streams.end()) {
        it->second->setPct(_pct);
        m_changed = true;
    }
}

void Scene::setStreamTime( const std::string& _name, float _time ) {
    TextureStreamsMap::iterator it = streams.find(_name);
    if (it != streams.end()) {
        it->second->setTime(_time);
        m_changed = true;
    }
}

void Scene::setStreamsPlay() {
    for (TextureStreamsMap::iterator it = streams.begin(); it != streams.end(); ++it) {
        it->second->play();
        m_changed = true;
    }
}

void Scene::setStreamsStop() {
    for (TextureStreamsMap::iterator it = streams.begin(); it != streams.end(); ++it) {
        it->second->stop();
        m_changed = true;
    }
}

void Scene::setStreamsRestart() {
    for (TextureStreamsMap::iterator it = streams.begin(); it != streams.end(); ++it) {
        it->second->restart();
        m_changed = true;
    }
}

void Scene::setStreamsFrame( size_t _frame) {
    for (TextureStreamsMap::iterator it = streams.begin(); it != streams.end(); ++it) {
        it->second->setFrame(_frame);
        m_changed = true;
    }
}

void Scene::setStreamsSpeed( float _speed ) {
    for (TextureStreamsMap::iterator it = streams.begin(); it != streams.end(); ++it) {
        it->second->setSpeed(_speed);
        m_changed = true;
    }
}

void Scene::setStreamsTime( float _time ) {
    for (TextureStreamsMap::iterator it = streams.begin(); it != streams.end(); ++it) {
        it->second->setTime(_time);
        m_changed = true;
    }
}

void Scene::setStreamsPct( float _pct ) {
    for (TextureStreamsMap::iterator it = streams.begin(); it != streams.end(); ++it) {
        it->second->setPct(_pct);
        m_changed = true;
    }
}

void Scene::setStreamsPrevs( size_t _total ) {
    m_streamsPrevs = _total;
    m_streamsPrevsChange = true;
    m_changed = true;
}

void Scene::printStreams() {
    for (TextureStreamsMap::iterator it = streams.begin(); it != streams.end(); ++it) {
        std::cout << "uniform sampler2D " << it->first << "; // " << it->second->getFilePath() << std::endl;

        if (m_streamsPrevs > 0)
            std::cout << "uniform sampler2D " << it->first << "Prev;" << std::endl;

        std::cout << "uniform float " << it->first+"CurrentFrame; //" << toString(it->second->getCurrentFrame(), 1) << std::endl;
        std::cout << "uniform float " << it->first+"TotalFrames;  //" << toString(it->second->getTotalFrames(), 1) << std::endl;
        std::cout << "uniform float " << it->first+"Time;         // " << toString(it->second->getTime(), 1) << std::endl;
        std::cout << "uniform float " << it->first+"Duration;     // " << toString(it->second->getDuration(), 1) << std::endl;
        std::cout << "uniform float " << it->first+"Fps;          // " << toString(it->second->getFps(), 1) << std::endl;
    }
}

// CUBEMAPS
// 
bool Scene::addCubemap( const std::string& _name, const std::string& _filename, bool _verbose ) {
    struct stat st;
    if ( stat(_filename.c_str(), &st) != 0 )
        std::cerr << "Error watching for cubefile: " << _filename << std::endl;
    
    else {
        TextureCube* tex = new TextureCube();
        if ( tex->load(_filename, true) ) {

            if (_verbose) {
                std::cout << "// " << _filename << " loaded as: " << std::endl;
                std::cout << "uniform samplerCube u_cubeMap;"<< std::endl;
                std::cout << "uniform vec3        u_SH[9];"<< std::endl;
            }

            cubemaps[_name] = tex;
            m_changed = true;
            return true;
        }
        else
            delete tex;
    }

    return false;
}

void Scene::printCubemaps() {
    std::cout << "// Cubemaps: " << std::endl;
    for (TextureCubesMap::iterator it = cubemaps.begin(); it != cubemaps.end(); ++it)
        std::cout << "// - " << it->first << std::endl;

    std::cout << "// Active Cubemap: " << std::endl;
    if (activeCubemap) {
        std::cout << "uniform samplerCube u_cubeMap; // " << activeCubemap->getFilePath() << std::endl;
        std::cout << "uniform vec3        u_SH[9]; // " << std::endl;
    }
}

void Scene::printCubemapSH(){
    if (activeCubemap) {
        for (size_t i = 0; i < 9; i++) {
            std::cout << "u_SH[" << i << "] = vec3(" << activeCubemap->SH[i].x << ", " << activeCubemap->SH[i].y << " , " << activeCubemap->SH[i].z << ");" << std::endl;
        }
    }
}

void Scene::clearCubemaps() {
    activeCubemap = nullptr;
    for (TextureCubesMap::iterator itr = cubemaps.begin(); itr != cubemaps.end(); ++itr)
        delete (itr->second);
    cubemaps.clear();
    m_changed = true;
}


// SKYBOX
//
void Scene::setSunPosition(float _az, float _elev, float _distance) {
    m_skybox.elevation = _elev;
    m_skybox.azimuth = _az;
    m_skybox.change = true;

    glm::vec3 p = glm::vec3(0.0f, 0.0f, _distance );
    glm::quat lat = glm::angleAxis(-m_skybox.elevation, glm::vec3(1.0, 0.0, 0.0));
    glm::quat lon = glm::angleAxis(m_skybox.azimuth, glm::vec3(0.0, 1.0, 0.0));
    p = lat * p;
    p = lon * p;
    lights["default"]->setPosition(p);

    m_changed = true;
}

void Scene::setSunPosition(const glm::vec3& _v) {
    m_skybox.elevation = atan2(_v.y, sqrt(_v.x * _v.x + _v.z * _v.z) );
    m_skybox.azimuth = atan2(_v.x, _v.z);
    m_skybox.change = true;
    lights["default"]->setPosition(_v);
    m_changed = true;
}

void Scene::setSkyTurbidity(float _turbidity) {
    m_skybox.turbidity = _turbidity;
    m_skybox.change = true;
    m_changed = true;
}

void Scene::setGroundAlbedo(const glm::vec3& _albedo) {
    m_skybox.groundAlbedo = _albedo;
    m_skybox.change = true;
    m_changed = true;
}

float Scene::getSunAzimuth() const { return m_skybox.azimuth; }
float Scene::getSunElevation() const { return m_skybox.elevation; }
float Scene::getSkyTurbidity() const { return m_skybox.turbidity; }
glm::vec3 Scene::getGroundAlbedo() const { return m_skybox.groundAlbedo; }

//  CAMERA
//
void Scene::printCameras() {
    std::cout << "// Cameras: " << std::endl;
    for (CamerasMap::iterator it = cameras.begin(); it != cameras.end(); ++it) {
        
        std::cout << "// " << it->first << ":" << std::endl; 
        std::cout << "//   position: " << toString( it->second->getPosition() ) << std::endl;
        std::cout << "//   target: " << toString( it->second->getTarget() ) << " distance: " << toString( it->second->getDistance() ) << std::endl;
        std::cout << "//   fov: " << toString( it->second->getFOV() ) << " near: " << toString( it->second->getNearClip() ) << " far: " << toString( it->second->getFarClip() ) << std::endl;
        std::cout << "// -------------------------------------------- " << std::endl; 

    }

    std::cout << "// Active Camera: " << std::endl;
    if (activeCamera)
        std::cout << "uniform vec3 u_camera; // " << toString( activeCamera->getPosition() ) << std::endl;
}

void Scene::clearCameras() {
    activeCamera = nullptr;
    for (CamerasMap::iterator it = cameras.begin(); it != cameras.end(); ++it)
        delete (it->second);
    cameras.clear();
    m_changed = true;
}

// LIGHTS
//
void Scene::printLights() {
    // Pass Light Uniforms
    if (lights.size() == 1) {
        LightsMap::iterator it = lights.begin();

        std::cout << "// " << it->first << std::endl;
        if (it->second->getLightType() != LIGHT_DIRECTIONAL)
            std::cout << "unifrom vect3 u_light; // " << toString( it->second->getPosition() ) << std::endl;
        std::cout << "unifrom vect3 u_lightColor; // " << toString( it->second->color )  << std::endl;
        if (it->second->getLightType() == LIGHT_DIRECTIONAL || it->second->getLightType() == LIGHT_SPOT)
            std::cout << "unifrom vect3 u_lightDirection; // " << toString( it->second->direction ) << std::endl;
        std::cout << "unifrom float u_lightIntensity; // " << toString( it->second->intensity, 3) << std::endl;
        if (it->second->falloff > 0.0)
            std::cout << "unifrom float u_lightFalloff; // " << toString( it->second->falloff, 3) << std::endl;
        
        // std::cout << "unifrom mat4 u_lightMatrix;";
        // std::cout << "unifrom sampler2D u_lightShadowMap; ";
    }
    else {
        // TODO:
        //      - Lights should be pass as structs?? 

        for (LightsMap::iterator it = lights.begin(); it != lights.end(); ++it) {
            std::string name = "u_" + it->first;
            std::cout << "// " << it->first << std::endl;
            if (it->second->getLightType() != LIGHT_DIRECTIONAL)
                std::cout << "uniform vec3 u_light; // " << toString( it->second->getPosition() ) << std::endl;
            std::cout << "uniform vec3 u_lightColor; // " << toString( it->second->color )  << std::endl;
            if (it->second->getLightType() == LIGHT_DIRECTIONAL || it->second->getLightType() == LIGHT_SPOT)
                std::cout << "uniform vec3 u_lightDirection; // " << toString( it->second->direction ) << std::endl;
            std::cout << "uniform float u_lightIntensity; // " << toString( it->second->intensity, 3) << std::endl;
            if (it->second->falloff > 0.0)
                std::cout << "uniform float u_lightFalloff; // " << toString( it->second->falloff, 3) << std::endl;
        }
    }
}

void Scene::clearLights() {
    for (LightsMap::iterator it = lights.begin(); it != lights.end(); ++it)
        delete (it->second);
    lights.clear();
    m_changed = true;
}

// MODEL
//
void Scene::printModels() {
    std::cout << "// Models: " << std::endl;
    for (ModelsMap::iterator it = models.begin(); it != models.end(); ++it)
        std::cout << "// " << it->first << std::endl;
}

void Scene::clearModels() {
    for (ModelsMap::iterator it = models.begin(); it != models.end(); ++it)
        delete (it->second);
    models.clear();
    m_changed = true;
}

// MATERIAL
// 
void Scene::printMaterials() {
    std::cout << "// Materials: " << std::endl;
    for (MaterialsMap::iterator it = materials.begin(); it != materials.end(); ++it)
        std::cout << "// " << it->first << std::endl;
}

void Scene::clearMaterials() {
    materials.clear();
    m_changed = true;
}

// Shaders
// 
void Scene::printShaders() {
    std::cout << "// Shaders: " << std::endl;
    for (ShadersMap::iterator it = shaders.begin(); it != shaders.end(); ++it)
        std::cout << "// " << it->first << std::endl;
}

void Scene::clearShaders() {
    for (ShadersMap::iterator it = shaders.begin(); it != shaders.end(); ++it)
        delete (it->second);
    shaders.clear();
    m_changed = true;
}

// FONTS
//
bool  Scene::addFont(const std::string& _name, const std::string& _path) {
    Font* newFont;
    FontsMap::iterator it = fonts.find(_name);
    if (it == fonts.end() ) {
        newFont = new Font();
        fonts[_name] = newFont;
        m_changed = true;
    }
    return newFont->load(_path);
}

Font* Scene::getDefaultFont() {
    FontsMap::iterator it = fonts.find("default");
    if (it != fonts.end())
        return it->second;

    Font* defaultFont = new Font();
    defaultFont->setAlign( ALIGN_CENTER );
    defaultFont->setAlign( ALIGN_BOTTOM );
    defaultFont->setSize(24.0f);
    defaultFont->setColor(glm::vec4(1.0));
    fonts["default"] = defaultFont;
    return defaultFont;
}

void  Scene::printFonts() {
    std::cout << "// Fonts: " << std::endl;
    for (FontsMap::iterator it = fonts.begin(); it != fonts.end(); ++it)
        std::cout << "// " << it->first << std::endl;
}

void  Scene::clearFonts() {
    for (FontsMap::iterator it = fonts.begin(); it != fonts.end(); ++it)
        delete (it->second);
    fonts.clear();
    m_changed = true;
}

// LABELS
//
void Scene::printLabels() {
    std::cout << "// Labels: " << std::endl;
    for (size_t i = 0; i < labels.size(); i++)
        std::cout << "// " << labels[i]->getText() << std::endl;
}

void Scene::clearLabels() {
    for (size_t i = 0; i < labels.size(); i++)
        delete labels[i];
    labels.clear();
    m_changed = true;
}

}