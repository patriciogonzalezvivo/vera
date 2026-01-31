#pragma once

#include <vector>
#include <string>
#include "glm/glm.hpp"

#include "vera/gl/texture.h"
#include "vera/gl/shader.h"
#include "camera.h"

namespace vera {

class Gsplat {
public:

    Gsplat();
    virtual ~Gsplat();

    bool    load(const std::string& _filepath);
    void    use(Shader* _shader);

    void    clear();
    size_t  count() const { return m_positions.size(); }
    
    void    render(Camera& _camera, glm::mat4 _model = glm::mat4(1.0f));

private:
    bool    loadPLY(const std::string& _filepath);
    bool    loadSPLAT(const std::string& _filepath);

    Texture* createTextureFloat();
    Texture* createTextureUint();

    void    sort(const glm::mat4& _viewProj);

    std::vector<glm::u8vec4>    m_colors;
    std::vector<glm::quat>      m_rotations;
    std::vector<glm::vec3>      m_positions;
    std::vector<glm::vec3>      m_normals;
    std::vector<glm::vec3>      m_scales;
    
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