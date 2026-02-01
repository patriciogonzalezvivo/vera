#pragma once

// DEFAULT SHADERS
// -----------------------------------------------------
const std::string default_scene_vert = R"(
#ifdef GL_ES
precision highp float;
#endif

uniform mat4        u_modelViewProjectionMatrix;
uniform mat4        u_projectionMatrix;
uniform mat4        u_modelMatrix;
uniform mat4        u_viewMatrix;
uniform mat3        u_normalMatrix;
uniform vec2        u_resolution;

#ifdef MODEL_PRIMITIVE_GSPLATS
uniform sampler2D   u_GsplatData;
uniform vec2        u_GsplatDataResolution; // Must be passed: vec2(4096.0, height)

uniform vec2        u_focal;

attribute vec2      a_position;
attribute float     a_index;
#else 

attribute vec4      a_position;
#endif

varying vec4        v_position;

#ifdef MODEL_VERTEX_COLOR
attribute vec4      a_color;
varying vec4        v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
attribute vec3      a_normal;
varying vec3        v_normal;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
attribute vec2      a_texcoord;
#endif
varying vec2        v_texcoord;

#ifdef MODEL_VERTEX_TANGENT
attribute vec4      a_tangent;
varying vec4        v_tangent;
varying mat3        v_tangentToWorld;
#endif

#ifdef LIGHT_SHADOWMAP
uniform mat4        u_lightMatrix;
varying vec4        v_lightCoord;
#endif

#include "lygia/math/transpose.glsl"
#include "lygia/math/toMat3.glsl"

void main(void) {

#ifdef MODEL_PRIMITIVE_GSPLATS
    v_texcoord = a_position;
    v_color = vec4(v_texcoord.x, v_texcoord.y, 0.0, 1.0);

    float width = u_GsplatDataResolution.x;
    float height = u_GsplatDataResolution.y;

    float fIndex = a_index;
    float row = floor(fIndex / 1024.0);
    float colStart = mod(fIndex, 1024.0) * 4.0;
    
    float v = (row + 0.5) / height;
    
    // p1: pos.xyz (position)
    vec4 p1 = texture2D(u_GsplatData, vec2((colStart + 0.5) / width, v));
    v_position = vec4(p1.xyz, 1.0);
    
    // Transform position to camera space
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
        
    // Covariance
    // p2: cov.xx, cov.xy, cov.xz, cov.yy
    // p3: cov.yz, cov.zz, 0, 0
    vec4 p2 = texture2D(u_GsplatData, vec2((colStart + 1.5) / width, v));
    vec4 p3 = texture2D(u_GsplatData, vec2((colStart + 2.5) / width, v));

    // p4: color.rgba
    vec4 p4 = texture2D(u_GsplatData, vec2((colStart + 3.5) / width, v));
    v_color = p4;
    
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
    
    // Compute final position
    vec2 vCenter = vec2(pos2d) / pos2d.w;
    vec2 pixel = 1.0 / u_resolution;
    gl_Position = vec4(
        vCenter + 
        a_position.x * majorAxis * pixel + 
        a_position.y * minorAxis * pixel,
        pos2d.z / pos2d.w, 1.0
    );

#else
    v_position = u_modelMatrix * a_position;
    v_texcoord = a_position.xy * 0.5 + 0.5;
    
    #ifdef MODEL_VERTEX_COLOR
    v_color = a_color;
    #endif
    
    #ifdef MODEL_VERTEX_NORMAL
    v_normal = vec4(u_modelMatrix * vec4(a_normal, 0.0) ).xyz;
    #endif
    
    #ifdef MODEL_VERTEX_TEXCOORD
    v_texcoord = a_texcoord;
    #endif
    
    #ifdef MODEL_VERTEX_TANGENT
    v_tangent = a_tangent;
    vec3 worldTangent = a_tangent.xyz;
    vec3 worldBiTangent = cross(v_normal, worldTangent);// * sign(a_tangent.w);
    v_tangentToWorld = mat3(normalize(worldTangent), normalize(worldBiTangent), normalize(v_normal));
    #endif
    
    gl_Position = u_projectionMatrix * u_viewMatrix * v_position;
#endif

#ifdef LIGHT_SHADOWMAP
    v_lightCoord = u_lightMatrix * v_position;
#endif
}
)";

const std::string default_scene_vert_300 = R"(
#ifdef GL_ES
precision highp float;
precision highp int;
precision highp usampler2D;
#endif

uniform mat4        u_modelViewProjectionMatrix;
uniform mat4        u_modelMatrix;
uniform mat4        u_viewMatrix;
uniform mat4        u_projectionMatrix;
uniform mat3        u_normalMatrix;
uniform vec2        u_resolution;

#ifdef MODEL_PRIMITIVE_GSPLATS
uniform usampler2D  u_GsplatData;
uniform vec2        u_focal;
in vec2             a_position;
in uint             a_index;

#else
in      vec4        a_position;
#endif
out     vec4        v_position;

#ifdef MODEL_VERTEX_COLOR
in      vec4        a_color;
out     vec4        v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
in      vec3        a_normal;
out     vec3        v_normal;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
in      vec2        a_texcoord;
#endif
out     vec2        v_texcoord;

#ifdef MODEL_VERTEX_TANGENT
in      vec4        a_tangent;
out     vec4        v_tangent;
out     mat3        v_tangentToWorld;
#endif

#ifdef LIGHT_SHADOWMAP
uniform mat4        u_lightMatrix;
out     vec4        v_lightCoord;
#endif

void main(void) {

#ifdef MODEL_PRIMITIVE_GSPLATS
    // Fetch gaussian data from texture
    uvec4 cen = texelFetch(u_GsplatData, ivec2((uint(a_index) & 0x3ffu) << 1, uint(a_index) >> 10), 0);
    
    // Transform position to camera space
    v_position = vec4(uintBitsToFloat(cen.xyz), 1.0);
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
    
    // Fetch covariance data
    uvec4 cov = texelFetch(u_GsplatData, ivec2(((uint(a_index) & 0x3ffu) << 1) | 1u, uint(a_index) >> 10), 0);
    
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
    
    vec2 vCenter = vec2(pos2d) / pos2d.w;
    vec2 pixel = 1.0 / u_resolution;
    gl_Position = vec4(
        vCenter + 
        a_position.x * majorAxis * pixel + 
        a_position.y * minorAxis * pixel,
        pos2d.z / pos2d.w, 1.0
    );

#else

    v_position = u_modelMatrix * a_position;
    v_texcoord = a_position.xy * 0.5 + 0.5;
    
    #ifdef MODEL_VERTEX_COLOR
    v_color = a_color;
    #endif
    
    #ifdef MODEL_VERTEX_NORMAL
    v_normal = vec4(u_modelMatrix * vec4(a_normal, 0.0) ).xyz;
    #endif
    
    #ifdef MODEL_VERTEX_TEXCOORD
    v_texcoord = a_texcoord;
    #endif
    
    #ifdef MODEL_VERTEX_TANGENT
    v_tangent = a_tangent;
    vec3 worldTangent = a_tangent.xyz;
    vec3 worldBiTangent = cross(v_normal, worldTangent);// * sign(a_tangent.w);
    v_tangentToWorld = mat3(normalize(worldTangent), normalize(worldBiTangent), normalize(v_normal));
    #endif
    
    
    gl_Position = u_projectionMatrix * u_viewMatrix * v_position;
#endif

#ifdef LIGHT_SHADOWMAP
    v_lightCoord = u_lightMatrix * v_position;
#endif
}
)";

const std::string default_scene_frag = R"(
#ifdef GL_ES
precision highp float;
#endif

uniform sampler2D   u_scene;
uniform sampler2D   u_sceneDepth;

uniform mat4        u_projectionMatrix;

uniform vec3        u_camera;
uniform float       u_cameraNearClip;
uniform float       u_cameraFarClip;

uniform vec3        u_light;
uniform vec3        u_lightColor;
uniform float       u_lightFalloff;
uniform float       u_lightIntensity;

uniform float       u_iblLuminance;

uniform samplerCube u_cubeMap;
uniform vec3        u_SH[9];

#ifdef LIGHT_SHADOWMAP
uniform sampler2D   u_lightShadowMap;
uniform mat4        u_lightMatrix;
varying vec4        v_lightCoord;
#endif

uniform vec2        u_resolution;
uniform float       u_time;

varying vec4        v_position;
varying vec4        v_color;
varying vec3        v_normal;

#ifdef MODEL_VERTEX_TEXCOORD
varying vec2        v_texcoord;
#endif

#ifdef MODEL_VERTEX_TANGENT
varying vec4        v_tangent;
varying mat3        v_tangentToWorld;
#endif

#define SURFACE_POSITION    v_position
#define CAMERA_POSITION     u_camera
#define IBL_LUMINANCE       u_iblLuminance

#define LIGHT_DIRECTION     u_light
#define LIGHT_COLOR         u_lightColor
#define LIGHT_FALLOFF       u_lightFalloff
#define LIGHT_INTENSITY     u_lightIntensity
#define LIGHT_COORD         v_lightCoord

#include "lygia/lighting/material/new.glsl"
#include "lygia/lighting/pbr.glsl"
#include "lygia/color/space/linear2gamma.glsl"

float checkBoard(vec2 uv, vec2 _scale) {
    uv = floor(fract(uv * _scale) * 2.0);
    return min(1.0, uv.x + uv.y) - (uv.x * uv.y);
}

void main(void) {
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    vec2 pixel = 1.0/u_resolution;
    vec2 st = gl_FragCoord.xy * pixel;
    
#ifdef MODEL_PRIMITIVE_GSPLATS
    float A = -dot(v_texcoord, v_texcoord);
    
    // Stricter clipping for finer edges
    if (A < -4.0) discard;
    
    // Use smoother attenuation curve
    float gaussian = exp(A);
    
    // Add edge smoothing to reduce aliasing
    float edgeSmoothness = smoothstep(-4.0, -3.5, A);
    float B = gaussian * v_color.a * edgeSmoothness;
    
    color.rgb = v_color.rgb;
    color.a   = B;

#else
    Material material = materialNew();
    #if defined(FLOOR) && defined(MODEL_VERTEX_TEXCOORD)
    material.albedo.rgb = vec3(0.5) + checkBoard(v_texcoord, vec2(8.0)) * 0.5;
    #endif

    color = pbr(material);
    color = linear2gamma(color);
#endif

    gl_FragColor = color;
}
)";
const std::string default_scene_frag_300 = R"(
#ifdef GL_ES
precision highp float;
#endif

uniform sampler2D   u_scene;
uniform sampler2D   u_sceneDepth;

uniform mat4        u_projectionMatrix;

uniform vec3        u_camera;
uniform float       u_cameraNearClip;
uniform float       u_cameraFarClip;

uniform vec3        u_light;
uniform vec3        u_lightColor;
uniform float       u_lightFalloff;
uniform float       u_lightIntensity;

uniform float       u_iblLuminance;

uniform samplerCube u_cubeMap;
uniform vec3        u_SH[9];

#ifdef LIGHT_SHADOWMAP
uniform sampler2D   u_lightShadowMap;
uniform mat4        u_lightMatrix;
int     vec4        v_lightCoord;
#endif

uniform vec2        u_resolution;
uniform float       u_time;

int     vec4        v_position;
int     vec4        v_color;
int     vec3        v_normal;

#ifdef MODEL_VERTEX_TEXCOORD
int     vec2        v_texcoord;
#endif

#ifdef MODEL_VERTEX_TANGENT
int     vec4        v_tangent;
int     mat3        v_tangentToWorld;
#endif

out vec4        fragColor;

#define SURFACE_POSITION    v_position
#define CAMERA_POSITION     u_camera
#define IBL_LUMINANCE       u_iblLuminance

#define LIGHT_DIRECTION     u_light
#define LIGHT_COLOR         u_lightColor
#define LIGHT_FALLOFF       u_lightFalloff
#define LIGHT_INTENSITY     u_lightIntensity
#define LIGHT_COORD         v_lightCoord

#include "lygia/lighting/material/new.glsl"
#include "lygia/lighting/pbr.glsl"
#include "lygia/color/space/linear2gamma.glsl"

float checkBoard(vec2 uv, vec2 _scale) {
    uv = floor(fract(uv * _scale) * 2.0);
    return min(1.0, uv.x + uv.y) - (uv.x * uv.y);
}

void main(void) {
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    vec2 pixel = 1.0/u_resolution;
    vec2 st = gl_FragCoord.xy * pixel;
    
#ifdef MODEL_PRIMITIVE_GSPLATS
    float A = -dot(v_texcoord, v_texcoord);
    
    // Stricter clipping for finer edges
    if (A < -4.0) discard;
    
    // Use smoother attenuation curve
    float gaussian = exp(A);
    
    // Add edge smoothing to reduce aliasing
    float edgeSmoothness = smoothstep(-4.0, -3.5, A);
    float B = gaussian * v_color.a * edgeSmoothness;
    
    color.rgb = v_color.rgb;
    color.a   = B;

#else
    Material material = materialNew();
    #if defined(FLOOR) && defined(MODEL_VERTEX_TEXCOORD)
    material.albedo.rgb = vec3(0.5) + checkBoard(v_texcoord, vec2(8.0)) * 0.5;
    #endif

    color = pbr(material);
    color = linear2gamma(color);
#endif

    fragColor = color;
})";
