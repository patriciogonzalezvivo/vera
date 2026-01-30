#pragma once

#include <string>

const std::string splat_vert = R"(
#ifdef GL_ES
precision highp float;
#endif

uniform sampler2D   u_tex0;
uniform vec2        u_tex0Resolution; // Must be passed: vec2(4096.0, height)

uniform mat4        u_projectionMatrix;
uniform mat4        u_viewMatrix;
uniform mat4        u_modelMatrix;
uniform mat3        u_normalMatrix;

uniform vec2        u_resolution;
uniform vec2        u_focal;

attribute vec2      a_position;
attribute float     a_index;

varying vec4        v_color;
varying vec2        v_texcoord;

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
    vec2 pixel = 1.0 / u_resolution;

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
    vec4 cam = u_viewMatrix * u_modelMatrix * vec4(p1.xyz, 1.0);
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
    
    mat3 T = transpose(toMat3(u_viewMatrix * u_modelMatrix)) * J;
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
    v_texcoord = a_position;
    
    // Compute final position
    vec2 vCenter = vec2(pos2d) / pos2d.w;
    gl_Position = vec4(
        vCenter + 
        a_position.x * majorAxis * pixel + 
        a_position.y * minorAxis * pixel,
        pos2d.z / pos2d.w, 1.0
    );
})";

static const std::string splat_vert_300 = R"(
#ifdef GL_ES
precision highp float;
#endif

uniform usampler2D  u_tex0;

uniform mat4        u_projectionMatrix;
uniform mat4        u_viewMatrix;
uniform mat4        u_modelMatrix;
uniform vec2        u_focal;
uniform vec2        u_resolution;

in vec2             a_position;
in uint             a_index;

out vec4            v_color;
out vec2            v_texcoord;

void main() {
    // Pixel size
    vec2 pixel = 1.0 / u_resolution;

    // Fetch gaussian data from texture
    uvec4 cen = texelFetch(u_tex0, ivec2((uint(a_index) & 0x3ffu) << 1, uint(a_index) >> 10), 0);
    
    // Transform position to camera space
    vec4 cam = u_viewMatrix * u_modelMatrix * vec4(uintBitsToFloat(cen.xyz), 1.0);
    vec4 pos2d = u_projectionMatrix * cam;
    
    // Frustum culling
    float clip = 1.2 * pos2d.w;
    if (pos2d.z < -pos2d.w || pos2d.z > pos2d.w || 
        pos2d.x < -clip || pos2d.x > clip || 
        pos2d.y < -clip || pos2d.y > clip) {
        gl_Position = vec4(0.0, 0.0, 2.0, 1.0);
        return;
    }
    
    // Fetch covariance data
    uvec4 cov = texelFetch(u_tex0, ivec2(((uint(a_index) & 0x3ffu) << 1) | 1u, uint(a_index) >> 10), 0);
    
    // Unpack half-precision covariance
    vec2 u1 = unpackHalf2x16(cov.x);
    vec2 u2 = unpackHalf2x16(cov.y);
    vec2 u3 = unpackHalf2x16(cov.z);
    mat3 Vrk = mat3(
        u1.x, u1.y, u2.x,
        u1.y, u2.y, u3.x,
        u2.x, u3.x, u3.y
    );
    
    // Compute 2D covariance
    mat3 J = mat3(
        u_focal.x / cam.z, 0.0, -(u_focal.x * cam.x) / (cam.z * cam.z),
        0.0, u_focal.y / cam.z, -(u_focal.y * cam.y) / (cam.z * cam.z),
        0.0, 0.0, 0.0
    );
    
    mat3 T = transpose(mat3(u_viewMatrix * u_modelMatrix)) * J;
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
    
    // Unpack color
    vec4 color = vec4(
        float((cov.w) & 0xffu),
        float((cov.w >> 8) & 0xffu),
        float((cov.w >> 16) & 0xffu),
        float((cov.w >> 24) & 0xffu)
    ) / 255.0;
    
    v_color = color;
    v_texcoord = a_position;
    
    // Compute final position
    vec2 vCenter = vec2(pos2d) / pos2d.w;
    gl_Position = vec4(
        vCenter + 
        a_position.x * majorAxis * pixel + 
        a_position.y * minorAxis * pixel,
        pos2d.z / pos2d.w, 1.0
    );
})";

const std::string splat_frag = R"(
#ifdef GL_ES
precision highp float;
#endif

varying vec4 v_color;
varying vec2 v_texcoord;

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
    float A = -dot(v_texcoord, v_texcoord);
    
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

const std::string splat_frag_300 = R"(
#ifdef GL_ES
precision highp float;
#endif

in vec4 v_color;
in vec2 v_texcoord;
    
out vec4 fragColor;

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
    float A = -dot(v_texcoord, v_texcoord);
    
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
    
    fragColor = vec4(color, B);
}
)";
