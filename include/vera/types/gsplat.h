#pragma once

#include <vector>
#include <string>
#include "glm/glm.hpp"

#include "vera/gl/texture.h"
#include "vera/gl/shader.h"
#include "camera.h"

namespace vera {

struct SplatBlock {
    glm::vec3 min_bounds;
    glm::vec3 max_bounds;
    std::vector<uint32_t> indices;
    unsigned int occlusionQuery = 0;
    bool occluded = false;
    int framesHidden = 0;
    bool queryIssued = false;
};

struct Frustum {
    glm::vec4 planes[6];
};

class Gsplat {
public:

    Gsplat();
    virtual ~Gsplat();

    bool    load(const std::string& _filepath);
    void    use(Shader* _shader);

    // By default, loadPLY()/loadSPLAT() rotate every splat 180 degrees
    // around X to turn COLMAP's (Y down, Z forward) convention into
    // OpenGL's (Y up, Z back) one -- the right thing for a splat viewed on
    // its own. But when COLMAP camera poses are also loaded (addCameras()),
    // those are placed in the raw, unflipped COLMAP frame (matching a plain
    // sparse-cloud .ply, which never gets this flip either), so flipping the
    // splat on top would rotate it 180 degrees away from its own cameras.
    // Set this before loading a splat to keep it in that same raw frame.
    static void setUseColmapFrame(bool _use) { s_useColmapFrame = _use; }
    static bool getUseColmapFrame() { return s_useColmapFrame; }

    void    clear();
    size_t  count() const { return m_positions.size(); }
    
    void    render(Camera* _camera, glm::mat4 _model = glm::mat4(1.0f), bool _sort = false);
    void    renderNormal(Camera* _camera, glm::mat4 _model = glm::mat4(1.0f), bool _sort = false);

    // Writes real per-splat depth (hardware depth test/write, not blended)
    // into whichever depth buffer is currently bound -- intended to be
    // called right alongside render() during the main scene pass, so it
    // contributes to the same depth buffer regular opaque geometry does
    // (e.g. backing u_sceneDepth). Only sufficiently opaque/solid splats
    // write depth (see splat_frag_depth); color writes are disabled for
    // this pass so it can't stomp the already-rendered color buffer.
    void    renderDepth(Camera* _camera, glm::mat4 _model = glm::mat4(1.0f), bool _sort = false);
    void    renderBlocks(Camera* _camera, glm::mat4 _model = glm::mat4(1.0f));

    void    setGridDim(int _dim);
    void    setOcclusionThreshold(int _threshold);
    void    setOcclusionScale(float _scale);

    int     getGridDim() const { return m_gridDim; }
    int     getOcclusionThreshold() const { return m_occlusionThreshold; }
    float   getOcclusionScale() const { return m_occlusionScale; }

    BoundingBox getBoundingBox() const;

    void    optimizeDataLayout();

private:
    static bool s_useColmapFrame;

    // Radix sort helper
    void    radixSort(std::vector<std::pair<float, uint32_t>>& arr);

    bool    loadPLY(const std::string& _filepath);
    bool    loadSPLAT(const std::string& _filepath);

    Texture* createTextureFloat();
    Texture* createTextureUint();

    void    buildSpatialIndex();
    void    performOcclusionQuery(const glm::mat4& _viewProj);
    void    sort(const glm::mat4& _viewProj);

    // Lazily creates/resizes the private depth-only FBO performOcclusionQuery()
    // draws its coarse block-shaped occluder proxies into, so that internal
    // culling heuristic never pollutes whatever depth buffer is actually
    // bound during render() (e.g. the main scene's, backing u_sceneDepth).
    void    ensureOcclusionFbo(int _width, int _height);

    // Lazy-init / shared-state helpers used by render()/renderNormal()/renderDepth()
    void    ensureColorShader();
    void    ensureNormalShader();
    void    ensureDepthShader();
    void    ensureSharedBuffers();
    void    ensureTexture(int _shaderVersion);
    void    ensureSorted(const glm::mat4& _viewProj, bool _sort);


    // Frustum helpers
    Frustum extractFrustum(const glm::mat4& _viewProj) const;
    bool    isBoxInFrustum(const glm::vec3& min, const glm::vec3& max, const Frustum& _frustum) const;

    int     m_gridDim               = 16;
    int     m_occlusionThreshold    = 10;
    float   m_occlusionScale        = 0.8f;


    std::vector<glm::u8vec4>    m_colors;
    std::vector<glm::quat>      m_rotations;
    std::vector<glm::vec3>      m_positions;
    std::vector<glm::vec3>      m_scales;
    
    std::vector<SplatBlock>     m_blocks;

    // Sorting cache to avoid reallocation
    std::vector<std::pair<float, uint32_t>> m_sorter;

    std::vector<float>      m_depthFloatIndex;
    std::vector<uint32_t>   m_depthUintIndex;

    // Tracks the viewProj matrix used for the last sort, so we can detect
    // camera/model changes ourselves instead of depending on Camera::bChange,
    // which may already have been consumed elsewhere earlier in the frame.
    glm::mat4               m_lastSortViewProj = glm::mat4(0.0f);
    bool                    m_hasSorted = false;


    std::vector<float>      m_worldPositions;   // Only needed for sorting

    Texture*                m_texture = nullptr;
    Shader*                 m_shader = nullptr;

    // Buffers (shared between the color and normal-buffer VAOs)
    GLuint                  m_vao = 0;
    GLuint                  m_positionVBO = 0;
    GLuint                  m_indexVBO = 0;

    // Cached attribute locations (color shader)
    GLint                   m_position = -1;
    GLint                   m_index = -1;

    bool                    m_borrowedShader = false;

    // Internal shader/VAO used to render the scene-normal buffer. Always
    // owned by Gsplat (never borrowed), since it has no scene-graph shader
    // equivalent to plug in (a splat's fragment layout is fixed).
    Shader*                 m_normalShader = nullptr;
    GLuint                  m_normalVao = 0;
    GLint                   m_normalPosition = -1;
    GLint                   m_normalIndex = -1;

    // Same as m_normalShader/m_normalVao, but for renderDepth().
    Shader*                 m_depthShader = nullptr;
    GLuint                  m_depthVao = 0;
    GLint                   m_depthPosition = -1;
    GLint                   m_depthIndex = -1;

    // Private depth-only FBO used exclusively by performOcclusionQuery() --
    // see ensureOcclusionFbo().
    GLuint                  m_occlusionFbo = 0;
    GLuint                  m_occlusionDepthTex = 0;
    int                     m_occlusionFboWidth = 0;
    int                     m_occlusionFboHeight = 0;
};

}