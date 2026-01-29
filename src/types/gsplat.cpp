#include "vera/types/gsplat.h"
#include "vera/ops/fs.h"

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
    if (m_texture) {
        m_texture->clear();
        delete m_texture;
        m_texture = nullptr;
    }

    if (m_shader) {
        delete m_shader;
        m_shader = nullptr;
    }
}

void Gsplat::clear() {
    m_positions.clear();
    m_scales.clear();
    m_rotations.clear();
    m_colors.clear();
    m_packedData.clear();
    m_worldPositions.clear();
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
        
        // .splat format often uses distinct quaternion ordering, usually w,x,y,z or x,y,z,w
        // Standard convention for this format seems to be: 
        // rot[0] = w, rot[1] = x, rot[2] = y, rot[3] = z? 
        // Or sometimes x,y,z,w
        // Let's assume standard antimatter15: rot[0] is X, ... ? 
        // Actually antimatter15 code: 
        // var u = (rot[0] - 128) / 128; var v = (rot[1] - 128) / 128; ...
        // q = new Quaternion(v, w, z, u); -> y, z, w, x ?? 
        
        // Let's assume generic construction for now, standard quaternion is (w, x, y, z)
        // Adjust based on visual results.
        
        // Common packing: rot_0, rot_1, rot_2, rot_3 -> x, y, z, w ? 
        // Let's iterate.
        glm::quat q(r3, r0, r1, r2); // w, x, y, z
        m_rotations[i] = glm::normalize(q);
    }

    pack();
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
    
    // Pack data for GPU
    pack();

    return true;
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

    if (m_texture) {
        m_texture->clear();
        delete m_texture;
        m_texture = nullptr;
    }
}

void Gsplat::initGPUData() {
    
    if (!m_texture) {
        m_texture = createTexture();
    }

    if (!m_shader) {
        m_shader = new Shader();

                std::string vertSrc = R"(#version 120

uniform sampler2D   u_tex0;
uniform vec2        u_tex0Resolution; // Must be passed: vec2(4096.0, height)

uniform mat4        u_projectionMatrix;
uniform mat4        u_viewMatrix;
uniform vec2        u_resolution;
uniform vec2        u_focal;

attribute vec2      a_position;
attribute float     a_index;

varying vec4        v_color;
varying vec2        v_position;

#if !defined(FNC_TRANSPOSE) && (__VERSION__ < 120)
#define FNC_TRANSPOSE
mat3 transpose(in mat3 m) {
    return mat3(    m[0][0], m[1][0], m[2][0],
                    m[0][1], m[1][1], m[2][1],
                    m[0][2], m[1][2], m[2][2] );
}

mat4 transpose(in mat4 m) {
    return mat4(    vec4(m[0][0], m[1][0], m[2][0], m[3][0]),
                    vec4(m[0][1], m[1][1], m[2][1], m[3][1]),
                    vec4(m[0][2], m[1][2], m[2][2], m[3][2]),
                    vec4(m[0][3], m[1][3], m[2][3], m[3][3])    );
}
#endif

#ifndef FNC_TOMAT3
#define FNC_TOMAT3
mat3 toMat3(mat4 m) {
    #if __VERSION__ >= 300
    return mat3(m);
    #else
    return mat3(m[0].xyz, m[1].xyz, m[2].xyz);
    #endif
}
#endif

void main() {
    float width = u_tex0Resolution.x;
    float height = u_tex0Resolution.y;

    // Fetch gaussian data from texture
    // Reconstruct texture coordinates from index
    float fIndex = a_index;
    float row = floor(fIndex / 1024.0);
    float colStart = mod(fIndex, 1024.0) * 4.0;
    
    // UVs center
    float v = (row + 0.5) / height;
    
    // Fetch 4 pixels
    vec4 p1 = texture2D(u_tex0, vec2((colStart + 0.5) / width, v));
    vec4 p2 = texture2D(u_tex0, vec2((colStart + 1.5) / width, v));
    vec4 p3 = texture2D(u_tex0, vec2((colStart + 2.5) / width, v));
    vec4 p4 = texture2D(u_tex0, vec2((colStart + 3.5) / width, v));

    // p1: pos.xyz, valid
    // p2: cov.xx, cov.xy, cov.xz, cov.yy
    // p3: cov.yz, cov.zz, 0, 0
    // p4: color.rgba

    // Transform position to camera space
    vec4 cam = u_viewMatrix * vec4(p1.xyz, 1.0);
    vec4 pos2d = u_projectionMatrix * cam;
    
    // Frustum culling
    float clip = 1.2 * pos2d.w;
    if (pos2d.z < -pos2d.w || pos2d.z > pos2d.w || 
        pos2d.x < -clip || pos2d.x > clip || 
        pos2d.y < -clip || pos2d.y > clip) {
        gl_Position = vec4(0.0, 0.0, 2.0, 1.0);
        return;
    }
    
    // Construct covariance matrix
    mat3 Vrk = mat3(
        p2.x, p2.y, p2.z,
        p2.y, p2.w, p3.x,
        p2.z, p3.x, p3.y
    );
    
    // Compute 2D covariance
    mat3 J = mat3(
        u_focal.x / cam.z, 0.0, -(u_focal.x * cam.x) / (cam.z * cam.z),
        0.0, u_focal.y / cam.z, -(u_focal.y * cam.y) / (cam.z * cam.z),
        0.0, 0.0, 0.0
    );
    
    mat3 T = transpose(toMat3(u_viewMatrix)) * J;
    mat3 cov2d = transpose(T) * Vrk * T;
    
    // Add low-pass filter (reduce value for finer splats)
    cov2d[0][0] += 0.1;
    cov2d[1][1] += 0.1;
    
    // Compute eigenvalues for ellipse
    float mid = (cov2d[0][0] + cov2d[1][1]) / 2.0;
    float radius = length(vec2((cov2d[0][0] - cov2d[1][1]) / 2.0, cov2d[0][1]));
    float lambda1 = mid + radius;
    float lambda2 = mid - radius;
    
    if (lambda2 < 0.0) {
        gl_Position = vec4(0.0, 0.0, 2.0, 1.0);
        return;
    }
    
    vec2 diagonalVector = normalize(vec2(cov2d[0][1], lambda1 - cov2d[0][0]));

    // Reduce scale for finer splat coverage
    float scale = 2.5;
    vec2 majorAxis = scale * min(sqrt(2.0 * lambda1), 1024.0) * diagonalVector;
    vec2 minorAxis = scale * min(sqrt(2.0 * lambda2), 1024.0) * vec2(diagonalVector.y, -diagonalVector.x);
    
    v_color = p4;
    v_position = a_position;
    
    // Compute final position
    vec2 vCenter = vec2(pos2d) / pos2d.w;
    gl_Position = vec4(
        vCenter + 
        a_position.x * majorAxis / u_resolution + 
        a_position.y * minorAxis / u_resolution,
        pos2d.z / pos2d.w, 1.0
    );
})";

        std::string fragSrc = R"(#version 120

varying vec4 v_color;
varying vec2 v_position;

// Function to increase saturation
vec3 adjustSaturation(vec3 color, float saturation) {
    const vec3 luminanceWeights = vec3(0.2126, 0.7152, 0.0722);
    float luminance = dot(color, luminanceWeights);
    return mix(vec3(luminance), color, saturation);
}

// White point adjustment (exposure and tone mapping)
vec3 adjustWhitePoint(vec3 color, float whitePoint) {
    // Use Reinhard tone mapping variant
    return color * (1.0 + color / (whitePoint * whitePoint)) / (1.0 + color);
}

void main() {
    float A = -dot(v_position, v_position);
    
    // Stricter clipping for finer edges
    if (A < -4.0) discard;
    
    // Use smoother attenuation curve
    float gaussian = exp(A);
    
    // Add edge smoothing to reduce aliasing
    float edgeSmoothness = smoothstep(-4.0, -3.5, A);
    float B = gaussian * v_color.a * edgeSmoothness;
    
    vec3 color = B * v_color.rgb;
    
    // Adjust saturation (1.0 = original, >1.0 = more saturated, <1.0 = desaturated)
    color = adjustSaturation(color, 1.2);
    
    // Adjust white point (lower value = brighter highlights)
    color = adjustWhitePoint(color, 0.9);
    
    // Slight sharpening effect to enhance details
    float sharpness = 1.05;
    color = pow(color, vec3(1.0 / sharpness));
    
    gl_FragColor = vec4(color, B);
})";

        // m_shader->load(loadGlslFrom("shaders/splat.frag"), loadGlslFrom("shaders/splat.vert"));
        m_shader->load(fragSrc, vertSrc);
    }

    if (m_vao == -1) {
    
        // Create VAO and VBOs
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);
        
        // Quad positions for point sprite
        float quadVertices[] = {
            -1.0f, -1.0f,
            1.0f, -1.0f,
            1.0f,  1.0f,
            -1.0f,  1.0f
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
            // Use glVertexAttribPointer to allow conversion to float in shader
            glVertexAttribPointer(m_index, 1, GL_FLOAT, GL_FALSE, 0, 0); 
            glVertexAttribDivisor(m_index, 1);
        }
    
        glBindVertexArray(0);
    }
}

Texture *Gsplat::createTexture() {

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
    texture->load(texWidth, texHeight, 4, 32, textureData.data(), NEAREST, CLAMP);

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
    
    // Extract sorted indices
    if (m_depthIndex.size() != vertexCount) {
        m_depthIndex.resize(vertexCount);
    }

    for (uint32_t i = 0; i < vertexCount; i++) {
        m_depthIndex[i] = m_sorter[i].second;
    }
}

void Gsplat::draw(Camera& _camera) {
    if (m_shader == nullptr || m_texture == nullptr ||
        m_vao == -1 || m_positionVBO == -1 || m_indexVBO == -1) {
        // initialize GPU data
        initGPUData();
    }

    // Sort splats by depth
    glm::mat4 viewProj = _camera.getProjectionMatrix() * _camera.getViewMatrix();
    sort(viewProj);

    // Update index buffer
    size_t splatCount = count();

    if (m_indexFloats.size() != splatCount)
        m_indexFloats.resize(splatCount);

    for (size_t i = 0; i < splatCount; i++)
        m_indexFloats[i] = static_cast<float>(m_depthIndex[i]);

    glBindBuffer(GL_ARRAY_BUFFER, m_indexVBO);
    glBufferData(GL_ARRAY_BUFFER, m_indexFloats.size() * sizeof(float), m_indexFloats.data(), GL_DYNAMIC_DRAW);

    m_shader->use();

    glBindVertexArray(m_vao);

    // Set uniforms
    m_shader->setUniformTexture("u_tex0", m_texture, 0); // Use member variable directly
    m_shader->setUniform("u_tex0Resolution", glm::vec2(m_texture->getWidth(), m_texture->getHeight()));

    glm::mat4 viewMatrix = _camera.getViewMatrix();
    glm::mat4 projMatrix = _camera.getProjectionMatrix();
    glm::vec2 viewportSize = glm::vec2(_camera.getViewport().z, _camera.getViewport().w);

    m_shader->setUniform("u_viewMatrix", viewMatrix);
    m_shader->setUniform("u_projectionMatrix", projMatrix);
    m_shader->setUniform("u_resolution", viewportSize);
    
    // Compute focal lengths from FOV
    float fovRad = glm::radians(_camera.getFOV());
    float fy = viewportSize.y / (2.0f * std::tan(fovRad / 2.0f));
    m_shader->setUniform("u_focal", glm::vec2(fy, fy));
    
    // Draw
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, count());
    glBindVertexArray(0);

    // Restore Render State
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

} // namespace vera