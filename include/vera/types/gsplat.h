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

// Coordinate frame the raw splat payload is authored in. Chosen per-source
// (standalone .splat/.ply vs a glTF node vs COLMAP), rather than the old
// global static -- see Gsplat::set().
enum GsplatFrame {
    GSPLAT_FRAME_FLIP_YZ = 0,   // rotate 180 deg about X (COLMAP Y-down/Z-forward -> OpenGL). Standalone .splat/.ply default.
    GSPLAT_FRAME_RAW,           // leave as authored (COLMAP raw frame, so a splat stays aligned with loaded COLMAP cameras)
    GSPLAT_FRAME_GLTF           // glTF is Y-up: no flip; the node transform passed to set() places the splat
};

// Number of higher-order (view-dependent) SH coefficients for a given degree,
// EXCLUDING the degree-0 DC term (which is baked into GsplatData::colors).
// deg 1 -> 3, deg 2 -> 8, deg 3 -> 15.
inline int gsplatSHCoeffCount(int _degree) {
    return _degree <= 0 ? 0 : (_degree + 1) * (_degree + 1) - 1;
}

// Source-agnostic splat payload. .splat, .ply-3DGS and glTF primitives all
// decode into this and feed Gsplat::set(), so data ingest is separated from
// file/format parsing (the single highest-leverage refactor for unified glTF
// loading). Values here are already in the format-neutral form the packer
// expects: linear per-axis scales, unit-ish quaternions, u8 RGBA base color.
// The frame flip and any node transform are applied by set(), not here.
struct GsplatData {
    std::vector<glm::vec3>      positions;
    std::vector<glm::vec3>      scales;      // linear (loaders undo exp/etc. before filling this)
    std::vector<glm::quat>      rotations;
    std::vector<glm::u8vec4>    colors;      // degree-0 / RGBA base color

    // Optional higher-order SH (view-dependent color), degree 1..3, RGB per
    // coefficient. Flattened per splat: sh[splat*gsplatSHCoeffCount(shDegree) + c].
    // Empty when shDegree == 0.
    std::vector<glm::vec3>      sh;
    int                        shDegree = 0;
};

class Gsplat {
public:

    Gsplat();
    virtual ~Gsplat();

    bool    load(const std::string& _filepath);

    // Ingest a decoded splat payload from any source (.splat/.ply/glTF).
    // Applies the per-source coordinate frame and an optional node/world
    // transform (positions get the full transform; rotations get its rotation
    // part; scales get its scale magnitude), then builds the render layout.
    bool    set(const GsplatData& _data, const glm::mat4& _transform = glm::mat4(1.0f), GsplatFrame _frame = GSPLAT_FRAME_FLIP_YZ);

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

    // Higher-order SH coefficients as an RGB float texture (one texel per
    // coefficient, gsplatSHCoeffCount(m_shDegree) columns per splat). Only
    // created when m_shDegree > 0. See createTextureFloat() for the tiling.
    Texture* createTextureSH();

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

    // Higher-order (view-dependent) SH, degree 1..3. Flattened per splat:
    // m_sh[i*gsplatSHCoeffCount(m_shDegree) + c]. Empty for degree-0 splats
    // (.splat, legacy .ply) -- see Phase 2 in TODO.md.
    std::vector<glm::vec3>      m_sh;
    int                         m_shDegree = 0;

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
    Texture*                m_shTexture = nullptr;   // higher-order SH (view-dependent color); null for degree-0 splats
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