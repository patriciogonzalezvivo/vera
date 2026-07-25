#pragma once

#include <string>

const std::string splat_vert = R"(
#ifdef GL_ES
precision highp float;
#endif

uniform sampler2D   u_GsplatData;
uniform vec2        u_GsplatDataResolution; // Must be passed: vec2(4096.0, height)

uniform mat4        u_projectionMatrix;
uniform mat4        u_viewMatrix;
uniform mat4        u_modelMatrix;
uniform mat3        u_normalMatrix;

uniform vec2        u_resolution;
uniform vec2        u_focal;

attribute vec2      a_position;
attribute float     a_index;

varying vec4        v_position;
varying vec4        v_color;
varying vec2        v_texcoord;
varying vec3        v_normal;
varying float       v_normalConfidence;

mat3 transpose(in mat3 m) {
    return mat3(    m[0][0], m[1][0], m[2][0],
                    m[0][1], m[1][1], m[2][1],
                    m[0][2], m[1][2], m[2][2] );
}

mat3 toMat3(mat4 m) {
    return mat3(m[0].xyz, m[1].xyz, m[2].xyz);
}

// Decodes a unit vector packed into 2 floats (signed octahedral encoding)
vec3 octDecode(vec2 f) {
    vec3 n = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    float t = max(-n.z, 0.0);
    n.x += n.x >= 0.0 ? -t : t;
    n.y += n.y >= 0.0 ? -t : t;
    return normalize(n);
}

void main() {
    float width = u_GsplatDataResolution.x;
    float height = u_GsplatDataResolution.y;
    vec2 pixel = 1.0 / u_resolution;

    // Fetch gaussian data from texture
    // Reconstruct texture coordinates from index
    float fIndex = a_index;
    float row = floor(fIndex / 1024.0);
    float colStart = mod(fIndex, 1024.0) * 4.0;
    
    // UVs center
    float v = (row + 0.5) / height;
    
    // Fetch 4 pixels
    vec4 p1 = texture2D(u_GsplatData, vec2((colStart + 0.5) / width, v));

    // p1: pos.xyz, normalConfidence
    // p2: cov.xx, cov.xy, cov.xz, cov.yy
    // p3: cov.yz, cov.zz, localNormal.oct(x,y)
    // p4: color.rgba

    v_normalConfidence = p1.w;

    // Transform position to camera space
    v_position = vec4(p1.xyz, 1.0);
    vec4 cam = u_viewMatrix * u_modelMatrix * v_position;
    vec4 pos2d = u_projectionMatrix * cam;
    
    // Frustum culling
    float clip = 1.2 * pos2d.w;
    if (pos2d.z < -pos2d.w || pos2d.z > pos2d.w || 
        pos2d.x < -clip || pos2d.x > clip || 
        pos2d.y < -clip || pos2d.y > clip) {
        gl_Position = vec4(0.0, 0.0, 2.0, 1.0);
        return;
    }
    
    vec4 p2 = texture2D(u_GsplatData, vec2((colStart + 1.5) / width, v));
    vec4 p3 = texture2D(u_GsplatData, vec2((colStart + 2.5) / width, v));
    vec4 p4 = texture2D(u_GsplatData, vec2((colStart + 3.5) / width, v));

    // p3.zw carries the splat's local-space "normal" (thinnest ellipsoid axis),
    // packed with signed octahedral encoding. Rotate it into view space and
    // flip it to face the camera (the sign of the thin axis is arbitrary).
    vec3 localNormal = octDecode(p3.zw);
    v_normal = normalize(toMat3(u_viewMatrix * u_modelMatrix) * localNormal);
    if (dot(v_normal, cam.xyz) > 0.0) v_normal = -v_normal;

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

const std::string splat_frag = R"(
#ifdef GL_ES
precision highp float;
#endif

varying vec4 v_position;
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
    
    vec3 color = v_color.rgb;

    // Adjust saturation (1.0 = original, >1.0 = more saturated, <1.0 = desaturated)
    color = adjustSaturation(color, 1.2);
    
    // Adjust white point (lower value = brighter highlights)
    color = adjustWhitePoint(color, 0.9);
    
    // Slight sharpening effect to enhance details
    float sharpness = 1.05;
    color = pow(color, vec3(1.0 / sharpness));
    
    gl_FragColor = vec4(color, B);
})";

const std::string splat_frag_normal = R"(
#ifdef GL_ES
precision highp float;
#endif

varying vec4 v_color;
varying vec2 v_texcoord;
varying vec3 v_normal;
varying float v_normalConfidence;

void main() {
    float A = -dot(v_texcoord, v_texcoord);
    if (A < -4.0) discard;

    float gaussian = exp(A);
    float edgeSmoothness = smoothstep(-4.0, -3.5, A);
    // Alpha-weighted like the color pass, and further down-weighted by how
    // much the thin-axis normal can be trusted (near-spherical or "stick"
    // shaped splats have no well-defined normal -- see splatNormalConfidence()).
    float B = gaussian * v_color.a * edgeSmoothness * v_normalConfidence;

    gl_FragColor = vec4(v_normal, B);
})";

const std::string splat_frag_depth = R"(
#ifdef GL_ES
precision highp float;
#endif

varying vec4 v_color;
varying vec2 v_texcoord;

void main() {
    float A = -dot(v_texcoord, v_texcoord);
    if (A < -4.0) discard;

    // Only write depth from the geometric CORE of the gaussian's footprint,
    // not its soft fuzzy tail -- keeps a clean per-splat surface instead of
    // spreading depth across the whole (very faint) visible extent. This is
    // deliberately independent of the splat's own opacity: real splats are
    // routinely individually semi-transparent and only look solid once many
    // overlapping ones blend together (same as the color pass), so gating
    // on opacity*gaussian the way the color pass's blend WEIGHT does would
    // silently exclude most legitimate splats from depth entirely. The
    // opacity check here is only a floor to skip genuinely negligible/near-
    // invisible splats. Real depth test/write is enabled for this pass (see
    // renderDepth()), so the GPU naturally keeps the nearest solid splat per
    // pixel; no sorting or blending needed here, unlike the color/normal
    // passes. gl_Position.z/w (and so the rasterized depth) is already the
    // exact splat-center depth from the shared vertex shader -- nothing to
    // compute or override here.
    if (exp(A) < 0.6 || v_color.a < 0.05) discard;

    gl_FragColor = vec4(0.0); // color unused -- color writes are disabled for this pass
})";

// GLSL ES 3.0 versions

static const std::string splat_vert_300 = R"(#version 300 es
precision highp float;
precision highp int;
precision highp usampler2D;

uniform usampler2D  u_GsplatData;

uniform mat4        u_projectionMatrix;
uniform mat4        u_viewMatrix;
uniform mat4        u_modelMatrix;
uniform vec2        u_focal;
uniform vec2        u_resolution;

in vec2             a_position;
in uint             a_index;

out vec4            v_position;
out vec4            v_color;
out vec2            v_texcoord;
out vec3            v_normal;
out float           v_normalConfidence;

// Decodes a unit vector packed into 2 floats (signed octahedral encoding)
vec3 octDecode(vec2 f) {
    vec3 n = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    float t = max(-n.z, 0.0);
    n.x += n.x >= 0.0 ? -t : t;
    n.y += n.y >= 0.0 ? -t : t;
    return normalize(n);
}

void main() {
    // Pixel size
    vec2 pixel = 1.0 / u_resolution;

    // Each splat occupies 3 columns (of the 4 reserved -- see createTextureUint())
    uint splatCol = (uint(a_index) & 0x3ffu) * 3u;
    uint splatRow = uint(a_index) >> 10;

    // Fetch gaussian data from texture
    uvec4 cen = texelFetch(u_GsplatData, ivec2(splatCol, splatRow), 0);

    // Transform position to camera space
    v_position = vec4(uintBitsToFloat(cen.xyz), 1.0);
    vec4 cam = u_viewMatrix * u_modelMatrix * v_position;
    vec4 pos2d = u_projectionMatrix * cam;

    // cen.w carries the splat's local-space "normal" (thinnest ellipsoid axis),
    // packed with signed octahedral encoding via packHalf2x16. Rotate it into
    // view space and flip it to face the camera (the axis sign is arbitrary).
    vec3 localNormal = octDecode(unpackHalf2x16(cen.w));
    v_normal = normalize(mat3(u_viewMatrix * u_modelMatrix) * localNormal);
    if (dot(v_normal, cam.xyz) > 0.0) v_normal = -v_normal;

    // Frustum culling
    float clip = 1.2 * pos2d.w;
    if (pos2d.z < -pos2d.w || pos2d.z > pos2d.w || 
        pos2d.x < -clip || pos2d.x > clip || 
        pos2d.y < -clip || pos2d.y > clip) {
        gl_Position = vec4(0.0, 0.0, 2.0, 1.0);
        return;
    }
    
    // Fetch covariance data and normal-confidence
    uvec4 cov = texelFetch(u_GsplatData, ivec2(splatCol + 1u, splatRow), 0);
    uvec4 conf = texelFetch(u_GsplatData, ivec2(splatCol + 2u, splatRow), 0);
    v_normalConfidence = uintBitsToFloat(conf.x);

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
}
)";


const std::string splat_frag_300 = R"(#version 300 es
precision highp float;
precision highp int;

in vec4 v_position;
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
    
    vec3 color = v_color.rgb;
    
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

const std::string splat_frag_normal_300 = R"(#version 300 es
precision highp float;
precision highp int;

in vec4 v_color;
in vec2 v_texcoord;
in vec3 v_normal;
in float v_normalConfidence;

out vec4 fragColor;

void main() {
    float A = -dot(v_texcoord, v_texcoord);
    if (A < -4.0) discard;

    float gaussian = exp(A);
    float edgeSmoothness = smoothstep(-4.0, -3.5, A);
    // Alpha-weighted like the color pass, and further down-weighted by how
    // much the thin-axis normal can be trusted (near-spherical or "stick"
    // shaped splats have no well-defined normal -- see splatNormalConfidence()).
    float B = gaussian * v_color.a * edgeSmoothness * v_normalConfidence;

    fragColor = vec4(v_normal, B);
}
)";

const std::string splat_frag_depth_300 = R"(#version 300 es
precision highp float;
precision highp int;

in vec4 v_color;
in vec2 v_texcoord;

out vec4 fragColor;

void main() {
    float A = -dot(v_texcoord, v_texcoord);
    if (A < -4.0) discard;

    // See splat_frag_depth (ES2 version) for the reasoning.
    if (exp(A) < 0.6 || v_color.a < 0.05) discard;

    fragColor = vec4(0.0);
}
)";
