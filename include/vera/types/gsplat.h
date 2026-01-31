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

    void    clear();
    size_t  count() const { return m_positions.size(); }
    
    void    renderDebug(Camera& _camera, glm::mat4 _model = glm::mat4(1.0f));
    void    render(Camera& _camera, glm::mat4 _model = glm::mat4(1.0f));

    void    setGridDim(int _dim);
    void    setOcclusionThreshold(int _threshold);
    void    setOcclusionScale(float _scale);

    int     getGridDim() const { return m_gridDim; }
    int     getOcclusionThreshold() const { return m_occlusionThreshold; }
    float   getOcclusionScale() const { return m_occlusionScale; }

    void    optimizeDataLayout();

private:
    // Radix sort helper
    void    radixSort(std::vector<std::pair<float, uint32_t>>& arr);

    bool    loadPLY(const std::string& _filepath);
    bool    loadSPLAT(const std::string& _filepath);

    Texture* createTextureFloat();
    Texture* createTextureUint();

    void    buildSpatialIndex();
    void    performOcclusionQuery(const glm::mat4& _viewProj);
    void    sort(const glm::mat4& _viewProj);
    
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


    std::vector<float>      m_worldPositions;   // Only needed for sorting

    Texture*                m_texture = nullptr;
    Shader*                 m_shader = nullptr;

    // Buffers
    GLuint                  m_vao = -1;
    GLuint                  m_positionVBO = -1;
    GLuint                  m_indexVBO = -1;

    // Cached attribute locations
    GLint                   m_position = -1;
    GLint                   m_index = -1;
};

}