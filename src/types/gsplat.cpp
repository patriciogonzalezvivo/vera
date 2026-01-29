#include "vera/types/gsplat.h"

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
}

void Gsplat::clear() {
    m_positions.clear();
    m_scales.clear();
    m_rotations.clear();
    m_colors.clear();
    m_packedData.clear();
    m_worldPositions.clear();
}

void Gsplat::load(const std::string& _filepath) {
    std::ifstream ss(_filepath, std::ios::binary);
    if (!ss.is_open()) {
        throw std::runtime_error("Failed to open PLY file: " + _filepath);
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
    
    // Pack data for GPU
    pack();
}

void Gsplat::pack() {
    size_t n = m_positions.size();
    
    // Allocate packed data - 4 vec4 per gaussian (16 floats)
    m_packedData.resize(n * 16); 
    m_worldPositions.resize(n * 3);
    
    for (size_t i = 0; i < n; i++) {
        const glm::vec3& pos = m_positions[i];
        const glm::vec3& scale = m_scales[i];
        const glm::quat& rot = m_rotations[i];
        const glm::u8vec4& color = m_colors[i];
        
        // Pixel 1: Position + selection
        m_packedData[i * 16 + 0] = pos.x;
        m_packedData[i * 16 + 1] = pos.y;
        m_packedData[i * 16 + 2] = pos.z;
        m_packedData[i * 16 + 3] = 1.0f; // selection flag (1.0 = valid)
        
        m_worldPositions[i * 3 + 0] = pos.x;
        m_worldPositions[i * 3 + 1] = pos.y;
        m_worldPositions[i * 3 + 2] = pos.z;
        
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
        m_packedData[i * 16 + 4] = sigma[0]; // xx
        m_packedData[i * 16 + 5] = sigma[1]; // xy
        m_packedData[i * 16 + 6] = sigma[2]; // xz
        m_packedData[i * 16 + 7] = sigma[3]; // yy

        // Pixel 3: Covariance part 2
        m_packedData[i * 16 + 8] = sigma[4]; // yz
        m_packedData[i * 16 + 9] = sigma[5]; // zz
        m_packedData[i * 16 + 10] = 0.0f;
        m_packedData[i * 16 + 11] = 0.0f;

        // Pixel 4: Color (normalized)
        m_packedData[i * 16 + 12] = color.r / 255.0f;
        m_packedData[i * 16 + 13] = color.g / 255.0f;
        m_packedData[i * 16 + 14] = color.b / 255.0f;
        m_packedData[i * 16 + 15] = color.a / 255.0f;
    }
}

void Gsplat::sort(const glm::mat4& _viewProj, std::vector<uint32_t>& _depthIndex) {
    // Compute depths
    size_t vertexCount = m_positions.size();
    std::vector<std::pair<float, uint32_t>> depths(vertexCount);
    
    for (uint32_t i = 0; i < vertexCount; i++) {
        glm::vec3 pos(
            m_worldPositions[i * 3 + 0],
            m_worldPositions[i * 3 + 1],
            m_worldPositions[i * 3 + 2]
        );
        
        glm::vec4 projected = _viewProj * glm::vec4(pos, 1.0f);
        float depth = projected.z / projected.w;
        
        depths[i] = {depth, i};
    }
    
    // Sort by depth (front to back for weighted blended transparency)
    std::sort(depths.begin(), depths.end(),
        [](const std::pair<float, uint32_t>& a, const std::pair<float, uint32_t>& b) { return a.first < b.first; });
    
    // Extract sorted indices
    _depthIndex.resize(vertexCount);
    _depthIndex.resize(vertexCount);
    for (uint32_t i = 0; i < vertexCount; i++) {
        _depthIndex[i] = depths[i].second;
    }
}

Texture *Gsplat::getTexture() {

    size_t splatCount = count();
    size_t splatsPerRow = 1024;
    size_t texWidth = splatsPerRow * 4;  // 4 texels per splat
    size_t texHeight = std::max(1, (int)std::ceil(splatCount / (float)splatsPerRow));

    // Reorganize packed data into 2D texture layout
    std::vector<float> textureData(texWidth * texHeight * 4, 0.0f);
    
    for (size_t i = 0; i < splatCount; i++) {
        int row = i / splatsPerRow;
        int col = (i % splatsPerRow) * 4;  // Each gaussian takes 4 columns
        
        // Pixel 1 (pos)
        size_t idx1 = (row * texWidth + col) * 4;
        textureData[idx1 + 0] = m_packedData[i * 16 + 0];
        textureData[idx1 + 1] = m_packedData[i * 16 + 1];
        textureData[idx1 + 2] = m_packedData[i * 16 + 2];
        textureData[idx1 + 3] = m_packedData[i * 16 + 3];
        
        // Pixel 2 (cov1)
        size_t idx2 = (row * texWidth + col + 1) * 4;
        textureData[idx2 + 0] = m_packedData[i * 16 + 4];
        textureData[idx2 + 1] = m_packedData[i * 16 + 5];
        textureData[idx2 + 2] = m_packedData[i * 16 + 6];
        textureData[idx2 + 3] = m_packedData[i * 16 + 7];

        // Pixel 3 (cov2)
        size_t idx3 = (row * texWidth + col + 2) * 4;
        textureData[idx3 + 0] = m_packedData[i * 16 + 8];
        textureData[idx3 + 1] = m_packedData[i * 16 + 9];
        textureData[idx3 + 2] = m_packedData[i * 16 + 10];
        textureData[idx3 + 3] = m_packedData[i * 16 + 11];

        // Pixel 4 (color)
        size_t idx4 = (row * texWidth + col + 3) * 4;
        textureData[idx4 + 0] = m_packedData[i * 16 + 12];
        textureData[idx4 + 1] = m_packedData[i * 16 + 13];
        textureData[idx4 + 2] = m_packedData[i * 16 + 14];
        textureData[idx4 + 3] = m_packedData[i * 16 + 15];
    }
    
    Texture* texture = new Texture();

    // GLuint texId = 0;
    // glGenTextures(1, &texId);
    // glBindTexture(GL_TEXTURE_2D, texId);
    
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, texWidth, texHeight, 0, GL_RGBA, GL_FLOAT, textureData.data());
        
    // texture->load(texWidth, texHeight, texId, NEAREST, CLAMP);
    texture->load(texWidth, texHeight, 4, 32, textureData.data(), NEAREST, CLAMP);

    return texture;

}

} // namespace vera