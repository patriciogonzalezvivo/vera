#include "vera/types/gsplat.h"
#include "vera/ops/fs.h"
#include "vera/shaders/gsplat.h"
#include "vera/shaders/defaultShaders.h"

#include "glm/gtc/quaternion.hpp"


#define TINYPLY_IMPLEMENTATION
#include "tinyply.h"

#include <cstring>
#include <fstream>
#include <algorithm>

// Spherical harmonics constant
constexpr float SH_C0 = 0.28209479177387814f;

// Pack two 16-bit halfs into a 32-bit uint
uint16_t floatToHalf(float value) {
    // Avoid type-punning UB by using memcpy
    uint32_t f = 0;
    std::memcpy(&f, &value, sizeof(uint32_t));
    uint32_t sign = (f >> 31) & 0x0001;
    uint32_t exp = (f >> 23) & 0x00ff;
    uint32_t frac = f & 0x007fffff;
    
    uint32_t newExp;
    if (exp == 0) {
        newExp = 0;
    } else if (exp < 113) {
        newExp = 0;
        frac |= 0x00800000;
        frac >>= (113 - exp);
        if (frac & 0x01000000) {
            newExp = 1;
            frac = 0;
        }
    } else if (exp < 142) {
        newExp = exp - 112;
    } else {
        newExp = 31;
        frac = 0;
    }
    
    return (sign << 15) | (newExp << 10) | (frac >> 13);
}

uint32_t packHalf2x16(float x, float y) {
    uint16_t hx = floatToHalf(x);
    uint16_t hy = floatToHalf(y);
    return (uint32_t)hx | ((uint32_t)hy << 16);
}


namespace vera {

Gsplat::Gsplat() { 
}

Gsplat::~Gsplat() {
    clear();
}

void Gsplat::clear() {
    m_positions.clear();
    m_scales.clear();
    m_rotations.clear();
    m_colors.clear();
    
    m_worldPositions.clear();

    m_sorter.clear();
    m_depthFloatIndex.clear();
    m_depthUintIndex.clear();

    if (m_texture) {
        m_texture->clear();
        delete m_texture;
        m_texture = nullptr;
    }

    if (m_shader) {
        delete m_shader;
        m_shader = nullptr;
    }

    // Clear GPU buffers
    if (m_vao != -1) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = -1;
    }

    if (m_positionVBO != -1) {
        glDeleteBuffers(1, &m_positionVBO);
        m_positionVBO = -1;
    }

    if (m_indexVBO != -1) {
        glDeleteBuffers(1, &m_indexVBO);
        m_indexVBO = -1;
    }

    if (m_position != -1) {
        m_position = -1;
    }

    if (m_index != -1) {
        m_index = -1;
    }
}

bool Gsplat::load(const std::string& _filepath) {
    std::string ext = _filepath.substr(_filepath.find_last_of(".") + 1);
    if (ext == "ply") {
        return loadPLY(_filepath);
    } else if (ext == "splat") {
        return loadSPLAT(_filepath);
    } else {
        // Fallback or error
        // Try PLY as default
        return loadPLY(_filepath);
    }
}

bool Gsplat::loadSPLAT(const std::string& _filepath) {
    std::ifstream file(_filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open SPLAT file: " + _filepath);
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // .splat format: 32 bytes per splat
    // pos(3*4) + scale(3*4) + color(4) + rot(4)
    size_t splatCount = size / 32;
    
    clear();
    m_positions.resize(splatCount);
    m_scales.resize(splatCount);
    m_rotations.resize(splatCount);
    m_colors.resize(splatCount);

    struct SplatData {
        float x, y, z;
        float sx, sy, sz;
        uint8_t r, g, b, a;
        uint8_t rot_0, rot_1, rot_2, rot_3;
    };

    std::vector<SplatData> buffer(splatCount);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
         return false;
    }

    for (size_t i = 0; i < splatCount; i++) {
        const SplatData& s = buffer[i];
        
        m_positions[i] = glm::vec3(s.x, s.y, s.z);
        m_scales[i] = glm::vec3(s.sx, s.sy, s.sz);
        
        // Color
        m_colors[i] = glm::u8vec4(s.r, s.g, s.b, s.a);
        
        // Rotation (mapping uint8 0..255 to -1.0..1.0)
        // (val - 128) / 128.0
        float r0 = (s.rot_0 - 128) / 128.0f;
        float r1 = (s.rot_1 - 128) / 128.0f;
        float r2 = (s.rot_2 - 128) / 128.0f;
        float r3 = (s.rot_3 - 128) / 128.0f;
        
        // Common packing: rot_0, rot_1, rot_2, rot_3 -> x, y, z, w ? 
        glm::quat q(r0, r1, r2, r3); // x, y, z, w
        // glm::quat q(r3, r0, r1, r2); // w, x, y, 
        m_rotations[i] = glm::normalize(q);
    }

    size_t n = m_positions.size();
    m_worldPositions.resize(n * 3);
    for (size_t i = 0; i < n; i++) {
        const glm::vec3& pos = m_positions[i];
        m_worldPositions[i * 3 + 0] = pos.x;
        m_worldPositions[i * 3 + 1] = pos.y;
        m_worldPositions[i * 3 + 2] = pos.z;
    }

    return true;
}

bool Gsplat::loadPLY(const std::string& _filepath) {
    std::ifstream ss(_filepath, std::ios::binary);
    if (!ss.is_open()) {
        throw std::runtime_error("Failed to open PLY file: " + _filepath);
        return false;
    }
    
    tinyply::PlyFile file;
    file.parse_header(ss);
    
    // Read vertex properties
    std::shared_ptr<tinyply::PlyData> vertices_x, vertices_y, vertices_z;
    std::shared_ptr<tinyply::PlyData> scale_0, scale_1, scale_2;
    std::shared_ptr<tinyply::PlyData> rot_0, rot_1, rot_2, rot_3;
    std::shared_ptr<tinyply::PlyData> f_dc_0, f_dc_1, f_dc_2;
    std::shared_ptr<tinyply::PlyData> opacity;
    std::shared_ptr<tinyply::PlyData> red, green, blue;
    
    try {
        vertices_x = file.request_properties_from_element("vertex", {"x"});
        vertices_y = file.request_properties_from_element("vertex", {"y"});
        vertices_z = file.request_properties_from_element("vertex", {"z"});
    } catch (const std::exception&) {
        throw std::runtime_error("PLY file missing position data");
    }
    
    // Try different naming conventions
    try {
        scale_0 = file.request_properties_from_element("vertex", {"scale_0"});
        scale_1 = file.request_properties_from_element("vertex", {"scale_1"});
        scale_2 = file.request_properties_from_element("vertex", {"scale_2"});
    } catch (const std::exception&) {
        try {
            scale_0 = file.request_properties_from_element("vertex", {"scaling_0"});
            scale_1 = file.request_properties_from_element("vertex", {"scaling_1"});
            scale_2 = file.request_properties_from_element("vertex", {"scaling_2"});
        } catch (const std::exception&) {
            throw std::runtime_error("PLY file missing scale data");
        }
    }
    
    try {
        rot_0 = file.request_properties_from_element("vertex", {"rot_0"});
        rot_1 = file.request_properties_from_element("vertex", {"rot_1"});
        rot_2 = file.request_properties_from_element("vertex", {"rot_2"});
        rot_3 = file.request_properties_from_element("vertex", {"rot_3"});
    } catch (const std::exception&) {
        try {
            rot_0 = file.request_properties_from_element("vertex", {"rotation_0"});
            rot_1 = file.request_properties_from_element("vertex", {"rotation_1"});
            rot_2 = file.request_properties_from_element("vertex", {"rotation_2"});
            rot_3 = file.request_properties_from_element("vertex", {"rotation_3"});
        } catch (const std::exception&) {
            throw std::runtime_error("PLY file missing rotation data");
        }
    }
    
    // Try to read colors
    bool hasSH = false;
    bool hasRGB = false;
    
    try {
        f_dc_0 = file.request_properties_from_element("vertex", {"f_dc_0"});
        f_dc_1 = file.request_properties_from_element("vertex", {"f_dc_1"});
        f_dc_2 = file.request_properties_from_element("vertex", {"f_dc_2"});
        hasSH = true;
    } catch (const std::exception&) {
        try {
            red = file.request_properties_from_element("vertex", {"red"});
            green = file.request_properties_from_element("vertex", {"green"});
            blue = file.request_properties_from_element("vertex", {"blue"});
            hasRGB = true;
        } catch (const std::exception&) {
            // Default white color
        }
    }
    
    try {
        opacity = file.request_properties_from_element("vertex", {"opacity"});
    } catch (const std::exception&) {
        // Default full opacity
    }
    
    file.read(ss);
    
    clear();

    size_t vertexCount = vertices_x->count;
    m_positions.resize(vertexCount);
    m_scales.resize(vertexCount);
    m_rotations.resize(vertexCount);
    m_colors.resize(vertexCount);
    
    const float* x_data = reinterpret_cast<const float*>(vertices_x->buffer.get());
    const float* y_data = reinterpret_cast<const float*>(vertices_y->buffer.get());
    const float* z_data = reinterpret_cast<const float*>(vertices_z->buffer.get());
    
    const float* scale_0_data = reinterpret_cast<const float*>(scale_0->buffer.get());
    const float* scale_1_data = reinterpret_cast<const float*>(scale_1->buffer.get());
    const float* scale_2_data = reinterpret_cast<const float*>(scale_2->buffer.get());
    
    const float* rot_0_data = reinterpret_cast<const float*>(rot_0->buffer.get());
    const float* rot_1_data = reinterpret_cast<const float*>(rot_1->buffer.get());
    const float* rot_2_data = reinterpret_cast<const float*>(rot_2->buffer.get());
    const float* rot_3_data = reinterpret_cast<const float*>(rot_3->buffer.get());
    
    const float* f_dc_0_data = hasSH ? reinterpret_cast<const float*>(f_dc_0->buffer.get()) : nullptr;
    const float* f_dc_1_data = hasSH ? reinterpret_cast<const float*>(f_dc_1->buffer.get()) : nullptr;
    const float* f_dc_2_data = hasSH ? reinterpret_cast<const float*>(f_dc_2->buffer.get()) : nullptr;
    
    const int* red_data = hasRGB ? reinterpret_cast<const int*>(red->buffer.get()) : nullptr;
    const int* green_data = hasRGB ? reinterpret_cast<const int*>(green->buffer.get()) : nullptr;
    const int* blue_data = hasRGB ? reinterpret_cast<const int*>(blue->buffer.get()) : nullptr;
    
    const float* opacity_data = opacity ? reinterpret_cast<const float*>(opacity->buffer.get()) : nullptr;
    
    for (size_t i = 0; i < vertexCount; i++) {
        // Position
        m_positions[i] = glm::vec3(x_data[i], y_data[i], z_data[i]);
        
        // Scale (exponential)
        m_scales[i] = glm::vec3(
            std::exp(scale_0_data[i]),
            std::exp(scale_1_data[i]),
            std::exp(scale_2_data[i])
        );
        
        // Rotation (quaternion - note: different convention)
        glm::quat q(rot_0_data[i], rot_1_data[i], rot_2_data[i], rot_3_data[i]);
        m_rotations[i] = glm::normalize(q);
        
        // Color
        uint8_t r, g, b, a;
        
        if (hasSH) {
            // Spherical harmonics to RGB
            r = static_cast<uint8_t>(glm::clamp((0.5f + SH_C0 * f_dc_0_data[i]) * 255.0f, 0.0f, 255.0f));
            g = static_cast<uint8_t>(glm::clamp((0.5f + SH_C0 * f_dc_1_data[i]) * 255.0f, 0.0f, 255.0f));
            b = static_cast<uint8_t>(glm::clamp((0.5f + SH_C0 * f_dc_2_data[i]) * 255.0f, 0.0f, 255.0f));
        } else if (hasRGB) {
            r = static_cast<uint8_t>(glm::clamp(red_data[i], 0, 255));
            g = static_cast<uint8_t>(glm::clamp(green_data[i], 0, 255));
            b = static_cast<uint8_t>(glm::clamp(blue_data[i], 0, 255));
        } else {
            r = g = b = 255;
        }
        
        // Opacity (sigmoid)
        if (opacity_data) {
            float op = 1.0f / (1.0f + std::exp(-opacity_data[i]));
            a = static_cast<uint8_t>(glm::clamp(op * 255.0f, 0.0f, 255.0f));
        } else {
            a = 255;
        }
        
        m_colors[i] = glm::u8vec4(r, g, b, a);
    }
    

    size_t n = m_positions.size();
    m_worldPositions.resize(n * 3);
    for (size_t i = 0; i < n; i++) {
        const glm::vec3& pos = m_positions[i];
        m_worldPositions[i * 3 + 0] = pos.x;
        m_worldPositions[i * 3 + 1] = pos.y;
        m_worldPositions[i * 3 + 2] = pos.z;
    }

    return true;
}

void Gsplat::use(Shader* _shader) {

    if (!_shader || _shader != m_shader) {

        bool different = false;
        if (m_shader && _shader->getProgram() != m_shader->getProgram()) {
            different = true;
        }

        if (m_shader && (
            _shader->getVersion() != m_shader->getVersion()
        ) ) {
            different = true;
            if (m_texture) {
                m_texture->clear();
                delete m_texture;
                m_texture = nullptr;
            }
        }
        m_shader = _shader;
     
        if (different) {
            // Invalidate VAO and VBOs
            if (m_vao != -1) {
                glDeleteVertexArrays(1, &m_vao);
                m_vao = -1;
            }

            if (m_positionVBO != -1) {
                glDeleteBuffers(1, &m_positionVBO);
                m_positionVBO = -1;
            }

            if (m_indexVBO != -1) {
                glDeleteBuffers(1, &m_indexVBO);
                m_indexVBO = -1;
            }
            m_position = -1;
            m_index = -1;
        }
    }

    if (m_vao == -1) {
        // Create VAO and VBOs
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);
        
        // Quad positions for point sprite
        float quadVertices[] = {
            -2.0f, -2.0f,
             2.0f, -2.0f,
             2.0f,  2.0f,
            -2.0f,  2.0f
        };
        
        glGenBuffers(1, &m_positionVBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_positionVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        
        m_position = m_shader->getAttribLocation("a_position");
        if (m_position != -1) {
            glEnableVertexAttribArray(m_position);
            glVertexAttribPointer(m_position, 2, GL_FLOAT, GL_FALSE, 0, 0);
        }

        // Index buffer (per-instance)
        glGenBuffers(1, &m_indexVBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_indexVBO);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

        m_index = m_shader->getAttribLocation("a_index");
        if (m_index != -1) {
            glEnableVertexAttribArray(m_index);

            if (m_shader->getVersion() >= 300) {
                // Use integer attribute for modern OpenGL
                glVertexAttribIPointer(m_index, 1, GL_UNSIGNED_INT, 0, 0);
            } else {
                // Use glVertexAttribPointer to allow conversion to float in shader
                glVertexAttribPointer(m_index, 1, GL_FLOAT, GL_FALSE, 0, 0); 
            }
            glVertexAttribDivisor(m_index, 1);
        }

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

Texture *Gsplat::createTextureFloat() {

    size_t splatCount = count();
    size_t splatsPerRow = 1024;
    size_t texWidth = splatsPerRow * 4;  // 4 texels per splat
    size_t texHeight = std::max(1, (int)std::ceil(splatCount / (float)splatsPerRow));

    // Allocate packed data - 4 vec4 per gaussian (16 floats)
    std::vector<float>  packedData;
    packedData.resize(splatCount * 16); 
    
    for (size_t i = 0; i < splatCount; i++) {
        const glm::vec3& pos = m_positions[i];
        const glm::vec3& scale = m_scales[i];
        const glm::quat& rot = m_rotations[i];
        const glm::u8vec4& color = m_colors[i];
        
        // Pixel 1: Position + selection
        packedData[i * 16 + 0] = pos.x;
        packedData[i * 16 + 1] = pos.y;
        packedData[i * 16 + 2] = pos.z;
        packedData[i * 16 + 3] = 1.0f; // selection flag (1.0 = valid)
        
        
        // Convert quaternion to matrix for covariance
        glm::mat3 rotMat = glm::mat3_cast(rot);
        glm::mat3 scaleMat(0.0f);
        scaleMat[0][0] = scale.x;
        scaleMat[1][1] = scale.y;
        scaleMat[2][2] = scale.z;
        
        glm::mat3 M = rotMat * scaleMat;
        
        // Compute 3D covariance (symmetric)
        float sigma[6];
        sigma[0] = M[0][0] * M[0][0] + M[1][0] * M[1][0] + M[2][0] * M[2][0]; // xx
        sigma[1] = M[0][0] * M[0][1] + M[1][0] * M[1][1] + M[2][0] * M[2][1]; // xy
        sigma[2] = M[0][0] * M[0][2] + M[1][0] * M[1][2] + M[2][0] * M[2][2]; // xz
        sigma[3] = M[0][1] * M[0][1] + M[1][1] * M[1][1] + M[2][1] * M[2][1]; // yy
        sigma[4] = M[0][1] * M[0][2] + M[1][1] * M[1][2] + M[2][1] * M[2][2]; // yz
        sigma[5] = M[0][2] * M[0][2] + M[1][2] * M[1][2] + M[2][2] * M[2][2]; // zz
        
        // Pixel 2: Covariance part 1
        packedData[i * 16 + 4] = sigma[0]; // xx
        packedData[i * 16 + 5] = sigma[1]; // xy
        packedData[i * 16 + 6] = sigma[2]; // xz
        packedData[i * 16 + 7] = sigma[3]; // yy

        // Pixel 3: Covariance part 2
        packedData[i * 16 + 8] = sigma[4]; // yz
        packedData[i * 16 + 9] = sigma[5]; // zz
        packedData[i * 16 + 10] = 0.0f;
        packedData[i * 16 + 11] = 0.0f;

        // Pixel 4: Color (normalized)
        packedData[i * 16 + 12] = color.r / 255.0f;
        packedData[i * 16 + 13] = color.g / 255.0f;
        packedData[i * 16 + 14] = color.b / 255.0f;
        packedData[i * 16 + 15] = color.a / 255.0f;
    }

    // Reorganize packed data into 2D texture layout
    std::vector<float> textureData(texWidth * texHeight * 4, 0.0f);
    
    for (size_t i = 0; i < splatCount; i++) {
        int row = i / splatsPerRow;
        int col = (i % splatsPerRow) * 4;  // Each gaussian takes 4 columns
        
        // Pixel 1 (pos)
        size_t idx1 = (row * texWidth + col) * 4;
        textureData[idx1 + 0] = packedData[i * 16 + 0];
        textureData[idx1 + 1] = packedData[i * 16 + 1];
        textureData[idx1 + 2] = packedData[i * 16 + 2];
        textureData[idx1 + 3] = packedData[i * 16 + 3];
        
        // Pixel 2 (cov1)
        size_t idx2 = (row * texWidth + col + 1) * 4;
        textureData[idx2 + 0] = packedData[i * 16 + 4];
        textureData[idx2 + 1] = packedData[i * 16 + 5];
        textureData[idx2 + 2] = packedData[i * 16 + 6];
        textureData[idx2 + 3] = packedData[i * 16 + 7];

        // Pixel 3 (cov2)
        size_t idx3 = (row * texWidth + col + 2) * 4;
        textureData[idx3 + 0] = packedData[i * 16 + 8];
        textureData[idx3 + 1] = packedData[i * 16 + 9];
        textureData[idx3 + 2] = packedData[i * 16 + 10];
        textureData[idx3 + 3] = packedData[i * 16 + 11];

        // Pixel 4 (color)
        size_t idx4 = (row * texWidth + col + 3) * 4;
        textureData[idx4 + 0] = packedData[i * 16 + 12];
        textureData[idx4 + 1] = packedData[i * 16 + 13];
        textureData[idx4 + 2] = packedData[i * 16 + 14];
        textureData[idx4 + 3] = packedData[i * 16 + 15];
    }
    
    Texture* texture = new Texture();
    texture->load(texWidth, texHeight, 4, 32, textureData.data(), NEAREST, CLAMP);

    return texture;
}

Texture* Gsplat::createTextureUint() {

    size_t splatCount = count();
    size_t splatsPerRow = 1024;
    size_t texWidth = splatsPerRow * 4;  // 4 texels per splat
    size_t texHeight = std::max(1, (int)std::ceil(splatCount / (float)splatsPerRow));

    std::vector<float>  packedData;
    packedData.resize(splatCount * 8);
    for (size_t i = 0; i < splatCount; i++) {
        const glm::vec3& pos = m_positions[i];
        const glm::vec3& scale = m_scales[i];
        const glm::quat& rot = m_rotations[i];
        const glm::u8vec4& color = m_colors[i];
        
        // First uvec4: position (xyz as floats reinterpreted as uint) + selection flag (w)
        // Safe bit copy without violating strict aliasing
        uint32_t ux, uy, uz;
        std::memcpy(&ux, &pos.x, sizeof(uint32_t));
        std::memcpy(&uy, &pos.y, sizeof(uint32_t));
        std::memcpy(&uz, &pos.z, sizeof(uint32_t));
        packedData[i * 8 + 0] = ux;
        packedData[i * 8 + 1] = uy;
        packedData[i * 8 + 2] = uz;
        packedData[i * 8 + 3] = 0; // selection flag (0 = not selected)
        
        // Convert quaternion to matrix for covariance
        glm::mat3 rotMat = glm::mat3_cast(rot);
        glm::mat3 scaleMat(0.0f);
        scaleMat[0][0] = scale.x;
        scaleMat[1][1] = scale.y;
        scaleMat[2][2] = scale.z;
        
        glm::mat3 M = rotMat * scaleMat;
        
        // Compute 3D covariance (symmetric)
        float sigma[6];
        sigma[0] = M[0][0] * M[0][0] + M[1][0] * M[1][0] + M[2][0] * M[2][0]; // xx
        sigma[1] = M[0][0] * M[0][1] + M[1][0] * M[1][1] + M[2][0] * M[2][1]; // xy
        sigma[2] = M[0][0] * M[0][2] + M[1][0] * M[1][2] + M[2][0] * M[2][2]; // xz
        sigma[3] = M[0][1] * M[0][1] + M[1][1] * M[1][1] + M[2][1] * M[2][1]; // yy
        sigma[4] = M[0][1] * M[0][2] + M[1][1] * M[1][2] + M[2][1] * M[2][2]; // yz
        sigma[5] = M[0][2] * M[0][2] + M[1][2] * M[1][2] + M[2][2] * M[2][2]; // zz
        
        // Second uvec4: covariance (xyz as half2) + color (w as packed RGBA)
        // Pack covariance as half-precision floats: xy, xz|yy, yz|zz
        packedData[i * 8 + 4] = packHalf2x16(sigma[0], sigma[1]); // xx, xy
        packedData[i * 8 + 5] = packHalf2x16(sigma[2], sigma[3]); // xz, yy
        packedData[i * 8 + 6] = packHalf2x16(sigma[4], sigma[5]); // yz, zz
        
        // Pack color as RGBA in a single uint32
        uint32_t packedColor = color.r | (color.g << 8) | (color.b << 16) | (color.a << 24);
        packedData[i * 8 + 7] = packedColor;
    }

    // Reorganize packed data into 2D texture layout
    // Each gaussian occupies 2 horizontal pixels (columns)
    std::vector<uint32_t> textureData(texWidth * texHeight * 4);
    for (size_t i = 0; i < splatCount; i++) {
        int row = i / 1024;
        int col = (i % 1024) * 2;  // Each gaussian takes 2 columns
        
        // First uvec4 (position + selection)
        int idx1 = (row * texWidth + col) * 4;
        textureData[idx1 + 0] = packedData[i * 8 + 0];
        textureData[idx1 + 1] = packedData[i * 8 + 1];
        textureData[idx1 + 2] = packedData[i * 8 + 2];
        textureData[idx1 + 3] = packedData[i * 8 + 3];
        
        // Second uvec4 (covariance + color)
        int idx2 = (row * texWidth + col + 1) * 4;
        textureData[idx2 + 0] = packedData[i * 8 + 4];
        textureData[idx2 + 1] = packedData[i * 8 + 5];
        textureData[idx2 + 2] = packedData[i * 8 + 6];
        textureData[idx2 + 3] = packedData[i * 8 + 7];
    }

    GLuint splatTexture;
    glGenTextures(1, &splatTexture);
    
    // Upload splat data texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, splatTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32UI, texWidth, texHeight, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, textureData.data());
                 
    Texture* texture = new Texture();
    texture->load(texWidth, texHeight, splatTexture, NEAREST, CLAMP);
    return texture;
}

void Gsplat::sort(const glm::mat4& _viewProj) {
    // Compute depths
    size_t vertexCount = m_positions.size();
    if (m_sorter.size() != vertexCount) {
        m_sorter.resize(vertexCount);
    }
    
    // Extract Matrix components for faster dot product (Column-Major access)
    // We want the result of (ViewProj * P).z and (ViewProj * P).w
    // (M * P).z = M[0].z*x + M[1].z*y + M[2].z*z + M[3].z
    // (M * P).w = M[0].w*x + M[1].w*y + M[2].w*z + M[3].w
    
    // Cache the relevant components
    float M02 = _viewProj[0][2]; float M12 = _viewProj[1][2]; float M22 = _viewProj[2][2]; float M32 = _viewProj[3][2];
    float M03 = _viewProj[0][3]; float M13 = _viewProj[1][3]; float M23 = _viewProj[2][3]; float M33 = _viewProj[3][3];
    
    for (uint32_t i = 0; i < vertexCount; i++) {
        // Unpack manually
        float x = m_worldPositions[i * 3 + 0];
        float y = m_worldPositions[i * 3 + 1];
        float z = m_worldPositions[i * 3 + 2];
        
        // Dot products manually expanded
        float pz = M02 * x + M12 * y + M22 * z + M32;
        float pw = M03 * x + M13 * y + M23 * z + M33;
        
        float depth = pz / pw;
        m_sorter[i] = {depth, i};
    }
    
    // Sort by depth (Back-to-Front for standard alpha blending)
    std::sort(m_sorter.begin(), m_sorter.end(),
        [](const std::pair<float, uint32_t>& a, const std::pair<float, uint32_t>& b) { return a.first > b.first; });

    if (m_shader) {
        if (m_shader->getVersion() >= 300) {
            // Use integer indices for modern OpenGL
            if (m_depthUintIndex.size() != vertexCount)
                m_depthUintIndex.resize(vertexCount);

            for (size_t i = 0; i < vertexCount; i++)
                m_depthUintIndex[i] = m_sorter[i].second;
        }
        else {
            // Use float indices for older OpenGL
            if (m_depthFloatIndex.size() != vertexCount)
                m_depthFloatIndex.resize(vertexCount);

            for (size_t i = 0; i < vertexCount; i++)
                m_depthFloatIndex[i] = static_cast<float>(m_sorter[i].second);
        }
    }
}

void Gsplat::render(Camera& _camera, glm::mat4 _model) {

    if (m_shader == nullptr) {        
        Shader* default_shader = new Shader();
        default_shader->load(getDefaultSrc(FRAG_SPLAT), getDefaultSrc(VERT_SPLAT));
        // default_shader->load(splat_frag, splat_vert);
        // default_shader->load(splat_frag_300, splat_vert_300);
        use(default_shader);
    }

    if (!m_texture) {
        if (m_shader->getVersion() >= 300) {
            m_texture = createTextureUint();
        }
        else {
            m_texture = createTextureFloat();
        }
    }

    // Sort splats by depth
    glm::mat4 viewProj = _camera.getProjectionMatrix() * _camera.getViewMatrix() * _model;
    sort(viewProj);

    // Update index buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_indexVBO);
    if (m_shader->getVersion() >= 300) {
        glBufferData(GL_ARRAY_BUFFER, m_depthUintIndex.size() * sizeof(uint32_t), m_depthUintIndex.data(), GL_STREAM_DRAW);
    }
    else {
        glBufferData(GL_ARRAY_BUFFER, m_depthFloatIndex.size() * sizeof(float), m_depthFloatIndex.data(), GL_DYNAMIC_DRAW);
    }

    m_shader->use();

    glBindVertexArray(m_vao);

    // Set uniforms
    m_shader->setUniformTexture("u_tex0", m_texture, 0); // Use member variable directly
    m_shader->setUniform("u_tex0Resolution", glm::vec2(m_texture->getWidth(), m_texture->getHeight()));

    m_shader->setUniform("u_modelMatrix", _model);
    m_shader->setUniform("u_normalMatrix", _camera.getNormalMatrix());
    m_shader->setUniform("u_viewMatrix", _camera.getViewMatrix());
    m_shader->setUniform("u_projectionMatrix", _camera.getProjectionMatrix());
    m_shader->setUniform("u_resolution", glm::vec2(_camera.getViewport().z, _camera.getViewport().w));
    
    // Compute focal lengths from FOV
    float fovRad = glm::radians(_camera.getFOV());
    float fy = _camera.getViewport().w / (2.0f * std::tan(fovRad / 2.0f));
    m_shader->setUniform("u_focal", glm::vec2(fy, fy));
    
    if (m_shader->getVersion() >= 300) {
        // Setup vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, m_positionVBO);
        glEnableVertexAttribArray(m_position);
        glVertexAttribPointer(m_position, 2, GL_FLOAT, GL_FALSE, 0, 0);
        
        glBindBuffer(GL_ARRAY_BUFFER, m_indexVBO);
        glEnableVertexAttribArray(m_index);
        glVertexAttribIPointer(m_index, 1, GL_UNSIGNED_INT, 0, 0);
        glVertexAttribDivisor(m_index, 1);    
    }

    // Draw
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, count());
    glBindVertexArray(0);

    // Unbind VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Unbind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace vera