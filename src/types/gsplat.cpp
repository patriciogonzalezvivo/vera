#include "vera/types/gsplat.h"
#include "vera/ops/fs.h"
#include "vera/ops/draw.h"
#include "vera/shaders/gsplat.h"
#include "vera/shaders/defaultShaders.h"

#include "glm/gtc/quaternion.hpp"


#define TINYPLY_IMPLEMENTATION
#include "tinyply.h"

#include <cstring>
#include <fstream>
#include <algorithm>
#include <numeric>

// Spherical harmonics constant
constexpr float SH_C0 = 0.28209479177387814f;

// Morton Encoding Helpers
inline uint32_t expandBits(uint32_t v) {
    v = (v * 0x00010001u) & 0xFF0000FFu;
    v = (v * 0x00000101u) & 0x0F00F00Fu;
    v = (v * 0x00000011u) & 0xC30C30C3u;
    v = (v * 0x00000005u) & 0x49249249u;
    return v;
}

inline uint32_t morton3D(const glm::vec3& pos, const glm::vec3& min, const glm::vec3& extents) {
    float x = (pos.x - min.x) / extents.x;
    float y = (pos.y - min.y) / extents.y;
    float z = (pos.z - min.z) / extents.z;
    
    x = std::min(std::max(x * 1024.0f, 0.0f), 1023.0f);
    y = std::min(std::max(y * 1024.0f, 0.0f), 1023.0f);
    z = std::min(std::max(z * 1024.0f, 0.0f), 1023.0f);
    
    uint32_t xx = expandBits((uint32_t)x);
    uint32_t yy = expandBits((uint32_t)y);
    uint32_t zz = expandBits((uint32_t)z);
    
    return xx | (yy << 1) | (zz << 2);
}

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

void Gsplat::setGridDim(int _dim) {
    if (m_gridDim != _dim) {
        m_gridDim = _dim;
        buildSpatialIndex();
    }
}

void Gsplat::setOcclusionThreshold(int _threshold) {
    m_occlusionThreshold = _threshold;
}

void Gsplat::setOcclusionScale(float _scale) {
    m_occlusionScale = _scale;
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
    size_t splatSize = 32;
    size_t splatCount = size / splatSize;
    
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

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
         return false;
    }

    for (size_t i = 0; i < splatCount; i++) {
        float x, y, z;
        float sx, sy, sz;
        uint8_t r, g, b, a;
        uint8_t rot_0, rot_1, rot_2, rot_3;
        float nx = 0.0f, ny = 0.0f, nz = 0.0f;

        const SplatData& s = reinterpret_cast<SplatData*>(buffer.data())[i];
        x = s.x; y = s.y; z = s.z;
        sx = s.sx; sy = s.sy; sz = s.sz;
        r = s.r; g = s.g; b = s.b; a = s.a;
        rot_0 = s.rot_0; rot_1 = s.rot_1; rot_2 = s.rot_2; rot_3 = s.rot_3;
        
        m_positions[i] = glm::vec3(x, -y, -z);
        m_scales[i] = glm::vec3(sx, sy, sz);
        
        // Color
        m_colors[i] = glm::u8vec4(r, g, b, a);
        
        // Rotation (mapping uint8 0..255 to -1.0..1.0)
        // (val - 128) / 128.0
        float r0 = (rot_0 - 128) / 128.0f;
        float r1 = (rot_1 - 128) / 128.0f;
        float r2 = (rot_2 - 128) / 128.0f;
        float r3 = (rot_3 - 128) / 128.0f;
        
        // Common packing: rot_0, rot_1, rot_2, rot_3 -> x, y, z, w ? 
        glm::quat q(r0, r1, r2, r3); // x, y, z, w
        
        // Rotate 180 degrees around X axis to match OpenGL coordinates
        static const glm::quat flipval(0.0f, 1.0f, 0.0f, 0.0f); 
        m_rotations[i] = glm::normalize(flipval * q);
    }

    optimizeDataLayout();

    size_t n = m_positions.size();
    m_worldPositions.resize(n * 3);
    for (size_t i = 0; i < n; i++) {
        const glm::vec3& pos = m_positions[i];
        m_worldPositions[i * 3 + 0] = pos.x;
        m_worldPositions[i * 3 + 1] = pos.y;
        m_worldPositions[i * 3 + 2] = pos.z;
    }

    buildSpatialIndex();

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
    std::shared_ptr<tinyply::PlyData> nx, ny, nz;
    
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
        m_positions[i] = glm::vec3(x_data[i], -y_data[i], -z_data[i]);
        
        // Scale (exponential)
        m_scales[i] = glm::vec3(
            std::exp(scale_0_data[i]),
            std::exp(scale_1_data[i]),
            std::exp(scale_2_data[i])
        );
        
        // Rotation (quaternion - note: different convention)
        glm::quat q(rot_0_data[i], rot_1_data[i], rot_2_data[i], rot_3_data[i]);

        // Rotate 180 degrees around X axis to match OpenGL coordinates
        static const glm::quat flipval(0.0f, 1.0f, 0.0f, 0.0f); 
        m_rotations[i] = glm::normalize(flipval * q);
        
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
    
    optimizeDataLayout();

    size_t n = m_positions.size();
    m_worldPositions.resize(n * 3);
    for (size_t i = 0; i < n; i++) {
        const glm::vec3& pos = m_positions[i];
        m_worldPositions[i * 3 + 0] = pos.x;
        m_worldPositions[i * 3 + 1] = pos.y;
        m_worldPositions[i * 3 + 2] = pos.z;
    }

    buildSpatialIndex();

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

    std::vector<uint32_t>  packedData;
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
        uint32_t packedColor = (uint32_t)color.r | ((uint32_t)color.g << 8) | ((uint32_t)color.b << 16) | ((uint32_t)color.a << 24);
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

void Gsplat::buildSpatialIndex() {
    m_blocks.clear();
    size_t count = m_positions.size();
    if (count == 0) return;

    // Calculate Scene Bounds
    glm::vec3 minB = m_positions[0];
    glm::vec3 maxB = m_positions[0];
    
    for (size_t i = 1; i < count; i++) {
        minB = glm::min(minB, m_positions[i]);
        maxB = glm::max(maxB, m_positions[i]);
    }
    
    // Create Grid
    m_blocks.resize(m_gridDim * m_gridDim * m_gridDim);
    
    glm::vec3 size = maxB - minB;
    size = glm::max(size, glm::vec3(0.001f));
    glm::vec3 step = size / (float)m_gridDim;

    // Initialize bounds
    for (int z = 0; z < m_gridDim; z++) {
        for (int y = 0; y < m_gridDim; y++) {
            for (int x = 0; x < m_gridDim; x++) {
                int idx = z * m_gridDim * m_gridDim + y * m_gridDim + x;
                m_blocks[idx].min_bounds = minB + glm::vec3(x, y, z) * step;
                m_blocks[idx].max_bounds = minB + glm::vec3(x+1, y+1, z+1) * step;
                m_blocks[idx].indices.reserve(count / m_blocks.size() * 2);
            }
        }
    }

    // Bin points
    for (uint32_t i = 0; i < count; i++) {
        const glm::vec3& p = m_positions[i];
        
        glm::vec3 relative = (p - minB) / size;
        relative = glm::clamp(relative, 0.0f, 0.9999f);
        
        int x = (int)(relative.x * m_gridDim);
        int y = (int)(relative.y * m_gridDim);
        int z = (int)(relative.z * m_gridDim);
        
        int idx = z * m_gridDim * m_gridDim + y * m_gridDim + x;
        m_blocks[idx].indices.push_back(i);
    }
    
    // Prune empty blocks
    std::vector<SplatBlock> packed;
    packed.reserve(m_blocks.size());
    
    // Setup queries
    bool supportQueries = false; // Check capability if needed, assuming yes for now or ignoring if 0
    // std::vector<GLuint> ids(m_blocks.size());
    // glGenQueries(m_blocks.size(), ids.data()); 
    // This might be too many queries for some drivers, but for 4096 blocks it's heavy.
    // Let's create queries only for non-empty blocks.

    for(size_t i = 0; i < m_blocks.size(); ++i) {
        if (!m_blocks[i].indices.empty()) {
            
            // Recalculate bounds to fit actual content
            glm::vec3 bMin = m_positions[m_blocks[i].indices[0]];
            glm::vec3 bMax = bMin;

            for (size_t k = 1; k < m_blocks[i].indices.size(); k++) {
                const glm::vec3& p = m_positions[m_blocks[i].indices[k]];
                bMin = glm::min(bMin, p);
                bMax = glm::max(bMax, p);
            }
            m_blocks[i].min_bounds = bMin;
            m_blocks[i].max_bounds = bMax;

            glGenQueries(1, &m_blocks[i].occlusionQuery);
            packed.push_back(m_blocks[i]);
        }
    }
    m_blocks = packed;
    std::cout << "Built Spatial Index: " << m_blocks.size() << " active blocks." << std::endl;
}

BoundingBox Gsplat::getBoundingBox() const {
    BoundingBox bbox;
    
    if (m_positions.empty())
        return bbox;

    glm::vec3 mean(0.0f);
    for (const auto& p : m_positions) mean += p;
    mean /= (float)m_positions.size();

    glm::vec3 accum(0.0f);
    for (const auto& p : m_positions) accum += (p - mean) * (p - mean);
    glm::vec3 dev = glm::sqrt(accum / (float)m_positions.size());

    // Filter outliers ( > 3 standard deviations )
    glm::vec3 limit = glm::max(dev * 3.0f, glm::vec3(0.00001f));

    for (const auto& p : m_positions) {
        glm::vec3 d = glm::abs(p - mean);
        if (d.x <= limit.x && d.y <= limit.y && d.z <= limit.z) {
            bbox.expand(p);
        }
    }

    return bbox;
}


void Gsplat::performOcclusionQuery(const glm::mat4& _viewProj) {
    if (m_blocks.empty()) return;

    Frustum frustum = extractFrustum(_viewProj);

    // 1. Check previous frame results - THIS UPDATES OCCLUSION STATE
    for (auto& block : m_blocks) {
        if (block.occlusionQuery == 0 || !block.queryIssued) continue;
        
        GLuint result = 0;
        GLuint available = 0;
        glGetQueryObjectuiv(block.occlusionQuery, GL_QUERY_RESULT_AVAILABLE, &available);
        if (available) {
            glGetQueryObjectuiv(block.occlusionQuery, GL_QUERY_RESULT, &result);
            
            if (result == 0) {
                block.framesHidden++;
                // Require N consecutive hidden frames to mark as occluded (Hysteresis)
                if (block.framesHidden > m_occlusionThreshold) {
                    block.occluded = true;
                }
            } else {
                block.framesHidden = 0;
                block.occluded = false; // Immediately visible
            }
        }
    }

    // Save state
    GLboolean colorMask[4];
    glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
    GLboolean depthMask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    GLboolean depthTest;
    glGetBooleanv(GL_DEPTH_TEST, &depthTest);
    GLint depthFunc;
    glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);

    // Filter visible blocks for depth pre-pass
    std::vector<SplatBlock*> visibleBlocks;
    visibleBlocks.reserve(m_blocks.size());
    for (auto& block : m_blocks) {
        if (!block.occluded && isBoxInFrustum(block.min_bounds, block.max_bounds, frustum)) {
            visibleBlocks.push_back(&block);
        }
    }

    vera::fill(255);
    vera::noStroke();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    // PASS 1: Fill Depth Buffer with Occluders (Visible Blocks)
    glDepthMask(GL_TRUE);

    for (auto* block : visibleBlocks) {
        glm::vec3 center = (block->min_bounds + block->max_bounds) * 0.5f;
        glm::vec3 size = block->max_bounds - block->min_bounds;
        glm::vec3 shrinkSize = size * m_occlusionScale;

        vera::push();
        vera::translate(center);
        vera::scale(shrinkSize);
        vera::box(1.0f);
        vera::pop();
    }

    // PASS 2: Issue Queries for All Blocks
    glDepthMask(GL_FALSE); // Read-only depth

    for (auto& block : m_blocks) {
        if (block.occlusionQuery == 0) continue;

        glm::vec3 center = (block.min_bounds + block.max_bounds) * 0.5f;
        glm::vec3 size = block.max_bounds - block.min_bounds;

        glBeginQuery(GL_ANY_SAMPLES_PASSED, block.occlusionQuery);
        vera::push();
        vera::translate(center);
        vera::scale(size);
        vera::box(1.0f);
        vera::pop();
        glEndQuery(GL_ANY_SAMPLES_PASSED);
        block.queryIssued = true;
    }

    // Restore state
    glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
    glDepthMask(depthMask);
    glDepthFunc(depthFunc);
    if (!depthTest) glDisable(GL_DEPTH_TEST);
}

Frustum Gsplat::extractFrustum(const glm::mat4& _viewProj) const {
    Frustum frustum;
    const glm::mat4& m = _viewProj;
    
    // Extract frustum planes (Gribb/Hartmann method)
    // Left
    frustum.planes[0].x = m[0][3] + m[0][0];
    frustum.planes[0].y = m[1][3] + m[1][0];
    frustum.planes[0].z = m[2][3] + m[2][0];
    frustum.planes[0].w = m[3][3] + m[3][0];
    
    // Right
    frustum.planes[1].x = m[0][3] - m[0][0];
    frustum.planes[1].y = m[1][3] - m[1][0];
    frustum.planes[1].z = m[2][3] - m[2][0];
    frustum.planes[1].w = m[3][3] - m[3][0];
    
    // Bottom
    frustum.planes[2].x = m[0][3] + m[0][1];
    frustum.planes[2].y = m[1][3] + m[1][1];
    frustum.planes[2].z = m[2][3] + m[2][1];
    frustum.planes[2].w = m[3][3] + m[3][1];
    
    // Top
    frustum.planes[3].x = m[0][3] - m[0][1];
    frustum.planes[3].y = m[1][3] - m[1][1];
    frustum.planes[3].z = m[2][3] - m[2][1];
    frustum.planes[3].w = m[3][3] - m[3][1];
    
    // Near
    frustum.planes[4].x = m[0][3] + m[0][2];
    frustum.planes[4].y = m[1][3] + m[1][2];
    frustum.planes[4].z = m[2][3] + m[2][2];
    frustum.planes[4].w = m[3][3] + m[3][2];
    
    // Far
    frustum.planes[5].x = m[0][3] - m[0][2];
    frustum.planes[5].y = m[1][3] - m[1][2];
    frustum.planes[5].z = m[2][3] - m[2][2];
    frustum.planes[5].w = m[3][3] - m[3][2];
    
    // Normalize planes
    for (int i = 0; i < 6; i++) {
        float len = glm::length(glm::vec3(frustum.planes[i]));
        frustum.planes[i] /= len;
    }
    
    return frustum;
}

bool Gsplat::isBoxInFrustum(const glm::vec3& min, const glm::vec3& max, const Frustum& _frustum) const {
    // Check if box is outside any of the 6 planes
    for (int i = 0; i < 6; i++) {
        // Find the p-vertex (direction of normal)
        glm::vec3 p;
        p.x = (_frustum.planes[i].x > 0) ? max.x : min.x;
        p.y = (_frustum.planes[i].y > 0) ? max.y : min.y;
        p.z = (_frustum.planes[i].z > 0) ? max.z : min.z;
        
        // If p-vertex is on the negative side of the plane, the box is outside
        if (glm::dot(glm::vec3(_frustum.planes[i]), p) + _frustum.planes[i].w < 0) {
            return false;
        }
    }
    return true;
}

/*
bool Gsplat::isBoxInFrustum(const glm::vec3& min, const glm::vec3& max, const glm::mat4& viewProj) {
    // Check Box Corners against Frustum Planes in Clip Space
    glm::vec4 corners[8];
    corners[0] = glm::vec4(min.x, min.y, min.z, 1.0f);
    corners[1] = glm::vec4(max.x, min.y, min.z, 1.0f);
    corners[2] = glm::vec4(min.x, max.y, min.z, 1.0f);
    corners[3] = glm::vec4(max.x, max.y, min.z, 1.0f);
    corners[4] = glm::vec4(min.x, min.y, max.z, 1.0f);
    corners[5] = glm::vec4(max.x, min.y, max.z, 1.0f);
    corners[6] = glm::vec4(min.x, max.y, max.z, 1.0f);
    corners[7] = glm::vec4(max.x, max.y, max.z, 1.0f);
    
    bool allXLeft = true;
    bool allXRight = true;
    bool allYBottom = true;
    bool allYTop = true;
    bool allZNear = true;
    bool allZFar = true;
    
    for(int i=0; i<8; i++) {
        glm::vec4 p = viewProj * corners[i];
        
        if (p.x >= -p.w) allXLeft = false;
        if (p.x <= p.w)  allXRight = false;
        if (p.y >= -p.w) allYBottom = false;
        if (p.y <= p.w)  allYTop = false;
        if (p.z >= -p.w) allZNear = false;
        if (p.z <= p.w)  allZFar = false; // Note: OpenGL near/far varies, but standard clip is -w..w
    }
    
    if (allXLeft || allXRight || allYBottom || allYTop || allZNear || allZFar) return false;
    
    return true;
}
*/

void Gsplat::sort(const glm::mat4& _viewProj) {
    m_sorter.clear();
    // Reserve to avoid reallocs
    m_sorter.reserve(m_positions.size()); 
    
    // Extract FRUSTUM
    Frustum frustum = extractFrustum(_viewProj);

    // View Proj Matrix components for depth calculation (using w_clip as depth approximation)
    float M03 = _viewProj[0][3]; float M13 = _viewProj[1][3]; float M23 = _viewProj[2][3]; float M33 = _viewProj[3][3];

    // If no blocks (e.g. failed load), fallback to all
    if (m_blocks.empty()) {
        size_t vertexCount = m_positions.size();
        for (uint32_t i = 0; i < vertexCount; i++) {
            float x = m_positions[i].x;
            float y = m_positions[i].y; 
            float z = m_positions[i].z;
            
            // Use w_clip as depth. (Distance from camera plane approx)
            float depth = M03 * x + M13 * y + M23 * z + M33;
            
            m_sorter.push_back({depth, i});
        }
    } else {
        // Block-based Culling
        for (auto& block : m_blocks) {
            bool visible = isBoxInFrustum(block.min_bounds, block.max_bounds, frustum);
            
            // Check Occlusion Result
            if (visible && block.occlusionQuery != 0 && block.queryIssued) {
                 GLuint available = 0;
                 glGetQueryObjectuiv(block.occlusionQuery, GL_QUERY_RESULT_AVAILABLE, &available);
                 if (available) {
                     GLuint samples = 0;
                     glGetQueryObjectuiv(block.occlusionQuery, GL_QUERY_RESULT, &samples);
                     if (samples == 0) {
                         block.occluded = true;
                         visible = false; // Aggressively cull
                     } else {
                         block.occluded = false;
                     }
                 } else {
                    // Result not ready, use previous state or assume visible
                    if (block.occluded) visible = false;
                 }
            }
            
            if (visible) {
                // Optimization: get pointers to data
                const uint32_t* indices = block.indices.data();
                size_t idxCount = block.indices.size();
                
                for (size_t k = 0; k < idxCount; k++) {
                    uint32_t i = indices[k];
                    // Direct access to local position (assuming model matrix handles transform)
                    float x = m_positions[i].x;
                    float y = m_positions[i].y;
                    float z = m_positions[i].z;
                    
                    float depth = M03 * x + M13 * y + M23 * z + M33;
                    m_sorter.push_back({depth, i});
                }
            }
        }
    }
    
    // Sort by depth (Back-to-Front)
    // Painter's Algo: Draw Farthest First.
    if (m_sorter.size() > 1000) {
        radixSort(m_sorter); 
    } else {
        std::sort(m_sorter.begin(), m_sorter.end(),
            [](const std::pair<float, uint32_t>& a, const std::pair<float, uint32_t>& b) { return a.first > b.first; });
    }

    size_t sortedCount = m_sorter.size();

    if (m_shader) {
        if (m_shader->getVersion() >= 300) {
            // Use integer indices for modern OpenGL
            if (m_depthUintIndex.size() != sortedCount)
                m_depthUintIndex.resize(sortedCount);

            for (size_t i = 0; i < sortedCount; i++)
                m_depthUintIndex[i] = m_sorter[i].second;
        }
        else {
            // Use float indices for older OpenGL
            if (m_depthFloatIndex.size() != sortedCount)
                m_depthFloatIndex.resize(sortedCount);

            for (size_t i = 0; i < sortedCount; i++)
                m_depthFloatIndex[i] = static_cast<float>(m_sorter[i].second);
        }
    }
}

// Radix sort implementation
void Gsplat::radixSort(std::vector<std::pair<float, uint32_t>>& arr) {
    if (arr.empty()) return;

    size_t n = arr.size();
    std::vector<std::pair<float, uint32_t>> buffer(n);
    
    // We treat the float key as uint32 to sort (handling sign bit if necessary, 
    // but assuming positive depth for simplicity or standard float sort tricks)
    // For sorting floats: 
    // - Positive floats sort correctly as uints
    // - Negative floats are reversed 
    // Here we likely deal with positive depths in view space (z < 0 usually but we sort by distance or -z)
    
    // 3 passes for 11 bits each? Or 4 passes of 8 bits. 4 passes is standard for 32-bit.
    
    std::pair<float, uint32_t>* input = arr.data();
    std::pair<float, uint32_t>* output = buffer.data();
    
    for (int shift = 0; shift < 32; shift += 8) {
        size_t count[256] = {0};
        
        // Histogram
        for (size_t i = 0; i < n; i++) {
            uint32_t f_bits;
            std::memcpy(&f_bits, &input[i].first, 4);
            
            // Flip sign bit for correct float sorting if needed
            // if (f_bits & 0x80000000) f_bits = ~f_bits; 
            // else f_bits |= 0x80000000; 
            // Simplification: We assume we sort by projected w or z which is generally > 0
            
            count[(f_bits >> shift) & 0xFF]++;
        }
        
        // Prefix sum
        size_t start[256];
        size_t total = 0;
        for (int i = 0; i < 256; i++) { // Normal order for Ascending? 
            // We want Back-to-Front (Descending sort)
            // So we can compute prefix sum backwards?
            // Or just sort Ascending and iterate backwards later?
            // Let's standard Ascending sort first.
            start[i] = total;
            total += count[i];
        }
        
        // Reorder
        for (size_t i = 0; i < n; i++) {
            uint32_t f_bits;
            std::memcpy(&f_bits, &input[i].first, 4);
            size_t pos = start[(f_bits >> shift) & 0xFF]++;
            output[pos] = input[i];
        }
        
        std::swap(input, output);
    }
    
    // Since we want Descending (Back-to-Front), and Radix sorts Ascending,
    // we just need to read the result in reverse, OR flip the logic.
    // However, since we write to GL buffers, best to have the vector sorted correctly.
    // Reversing the vector is cheap-ish or we can invert the key.
    
    // Result is in 'input' (which might be arr or buffer)
    if (input != arr.data()) {
        std::memcpy(arr.data(), buffer.data(), n * sizeof(std::pair<float, uint32_t>));
    }
    
    // Reverse for Back-to-front
    std::reverse(arr.begin(), arr.end());
}


void Gsplat::render(Camera* _camera, glm::mat4 _model, bool _sort) {
    if (!_camera)
        return;

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
    glm::mat4 viewProj = _camera->getProjectionMatrix() * _camera->getViewMatrix() * _model;
    
    bool needsSort = _camera->bChange || _sort;
    // Also check if valid indices exist (first frame)
    if (m_depthUintIndex.empty() && m_depthFloatIndex.empty()) {
        needsSort = true;
    }
    
    if (needsSort) {
        sort(viewProj);
    }

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
    m_shader->setUniform("u_normalMatrix", _camera->getNormalMatrix());
    m_shader->setUniform("u_viewMatrix", _camera->getViewMatrix());
    m_shader->setUniform("u_projectionMatrix", _camera->getProjectionMatrix());
    m_shader->setUniform("u_resolution", glm::vec2(_camera->getViewport().z, _camera->getViewport().w));
    
    // Compute focal lengths from FOV
    float fovRad = glm::radians(_camera->getFOV());
    float fy = _camera->getViewport().w / (2.0f * std::tan(fovRad / 2.0f));
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
    GLboolean depthMask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    glDepthMask(GL_FALSE);  // Disable depth writes for transparency

    size_t drawCount = (m_shader->getVersion() >= 300) ? m_depthUintIndex.size() : m_depthFloatIndex.size();
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, drawCount);
    
    glDepthMask(depthMask); // Restore depth mask

    glBindVertexArray(0);

    // Unbind VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Unbind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // After render, we issue occlusion queries for next frame
    performOcclusionQuery(viewProj);
}

void Gsplat::renderDebug(Camera* _camera, glm::mat4 _model) {
    if (!_camera)
        return;

    if (m_blocks.empty()) 
        return;

    vera::noFill();
    vera::strokeWeight(1.0f);

    glm::mat4 viewProj = _camera->getProjectionMatrix() * _camera->getViewMatrix() * _model;
    Frustum frustum = extractFrustum(viewProj);

    for (const auto& block : m_blocks) {
        bool visible = isBoxInFrustum(block.min_bounds, block.max_bounds, frustum);
        bool occluded = block.occluded;
        
        if (visible && !occluded) {
            vera::stroke(0.0f, 1.0f, 0.0f, 1.0f);
        } else {
            vera::stroke(1.0f, 0.0f, 0.0f, 0.3f);
        }

        vera::BoundingBox bbox;
        bbox.min = block.min_bounds;
        bbox.max = block.max_bounds;
        vera::line(bbox);
    }
}

void Gsplat::optimizeDataLayout() {
    size_t count = m_positions.size();
    if (count == 0) return;

    // 1. Calculate Scene Bounds
    glm::vec3 minB = m_positions[0];
    glm::vec3 maxB = m_positions[0];
    for (size_t i = 1; i < count; i++) {
        minB = glm::min(minB, m_positions[i]);
        maxB = glm::max(maxB, m_positions[i]);
    }
    glm::vec3 extents = maxB - minB;
    extents = glm::max(extents, glm::vec3(0.001f));

    // 2. Generate Sort Keys (Morton Codes)
    std::vector<uint32_t> indices(count);
    std::iota(indices.begin(), indices.end(), 0);
    
    std::vector<uint32_t> keys(count);
    for (size_t i = 0; i < count; i++) {
        keys[i] = morton3D(m_positions[i], minB, extents);
    }

    // 3. Sort indices based on keys
    std::sort(indices.begin(), indices.end(), [&](uint32_t a, uint32_t b) {
        return keys[a] < keys[b];
    });

    // 4. Apply Permutation to all vectors
    auto permute = [&](auto& vec) {
        using T = typename std::decay<decltype(vec)>::type::value_type;
        if (vec.empty()) return;
        std::vector<T> temp = vec;
        for (size_t i = 0; i < count; i++) {
            vec[i] = temp[indices[i]];
        }
    };

    permute(m_positions);
    permute(m_scales);
    permute(m_rotations);
    permute(m_colors);
}

} // namespace vera