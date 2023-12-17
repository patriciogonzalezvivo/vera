#pragma once

#include <string>

const std::string devlook_billboard_vert = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform vec2    u_resolution;

attribute vec4  a_position;
varying vec4    v_position;

#ifdef MODEL_VERTEX_COLOR
attribute vec4  a_color;
varying vec4    v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
attribute vec3  a_normal;
varying vec3    v_normal;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
attribute vec2  a_texcoord;
#endif
varying vec2    v_texcoord;

#ifdef MODEL_VERTEX_TANGENT
attribute vec4  a_tangent;
varying vec4    v_tangent;
varying mat3    v_tangentToWorld;
#endif

#ifdef LIGHT_SHADOWMAP
uniform mat4    u_lightMatrix;
varying vec4    v_lightCoord;
#endif

mat4 orthographic(float l, float r, float b, float t, float n, float f) {
    return mat4(
        vec4(2.0/(r-l),     0.0,          0.0,         0.0),
        vec4(0.0,           2.0/(t-b),    0.0,         0.0),
        vec4(0.0,           0.0,         -2.0/(f-n),   0.0),
        vec4(-(r+l)/(r-l), -(t+b)/(t-b), -(f+n)/(f-n), 1.0)
    );
}

void main(void) {
    v_position = a_position;

#ifdef MODEL_VERTEX_COLOR
    v_color = a_color;
#endif
    
#ifdef MODEL_VERTEX_NORMAL
    v_normal = a_normal;
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

#ifdef LIGHT_SHADOWMAP
    v_lightCoord = vec4(0.0);//u_lightMatrix * v_position;
#endif

    float area = 2.0;
    mat4 P = orthographic(  area, -area, 
                            area, -area, 
                            -1.0, 5.0);

    float aspect = u_resolution.y / u_resolution.x;

    gl_Position = P * a_position ;
    gl_Position.xy *= vec2(aspect, 1.0) * 0.65;
    gl_Position.x -= 0.80;
    gl_Position.y += DEVLOOK_Y_OFFSET;
})";

const std::string devlook_billboard_vert_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;

in  vec4    a_position;
out vec4    v_position;

#ifdef MODEL_VERTEX_COLOR
in  vec4    a_color;
out vec4    v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
in  vec3    a_normal;
out vec3    v_normal;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
in  vec2    a_texcoord;
#endif
out vec2    v_texcoord;

#ifdef MODEL_VERTEX_TANGENT
in  vec4    a_tangent;
out vec4    v_tangent;
out mat3    v_tangentToWorld;
#endif

#ifdef LIGHT_SHADOWMAP
uniform mat4    u_lightMatrix;
out vec4    v_lightCoord;
#endif

mat4 orthographic(float l, float r, float b, float t, float n, float f) {
    return mat4(
        vec4(2.0/(r-l),     0.0,          0.0,         0.0),
        vec4(0.0,           2.0/(t-b),    0.0,         0.0),
        vec4(0.0,           0.0,         -2.0/(f-n),   0.0),
        vec4(-(r+l)/(r-l), -(t+b)/(t-b), -(f+n)/(f-n), 1.0)
    );
}

void main(void) {
    v_position = a_position;

#ifdef MODEL_VERTEX_COLOR
    v_color = a_color;
#endif
    
#ifdef MODEL_VERTEX_NORMAL
    v_normal = a_normal;
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

#ifdef LIGHT_SHADOWMAP
    v_lightCoord = vec4(0.0);//u_lightMatrix * v_position;
#endif

    float area = 2.0;
    mat4 P = orthographic(  area, -area, 
                            area, -area, 
                            -1.0, 5.0);

    float aspect = u_resolution.y / u_resolution.x;

    gl_Position = P * a_position ;
    gl_Position.xy *= vec2(aspect, 1.0) * 0.65;
    gl_Position.x -= 0.80;
    gl_Position.y += DEVLOOK_Y_OFFSET;
})";

const std::string devlook_sphere_vert = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform vec3    u_camera;
uniform vec2    u_resolution;

attribute vec4  a_position;
varying vec4    v_position;

#ifdef MODEL_VERTEX_COLOR
attribute vec4  a_color;
varying vec4    v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
attribute vec3  a_normal;
varying vec3    v_normal;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
attribute vec2  a_texcoord;
#endif
varying vec2    v_texcoord;

#ifdef MODEL_VERTEX_TANGENT
attribute vec4  a_tangent;
varying vec4    v_tangent;
varying mat3    v_tangentToWorld;
#endif

#ifdef LIGHT_SHADOWMAP
uniform mat4    u_lightMatrix;
varying vec4    v_lightCoord;
#endif

mat4 toMat4(mat3 m) {
    return mat4(vec4(m[0], 0.0), 
                vec4(m[1], 0.0), 
                vec4(m[2], 0.0), 
                vec4(0.0, 0.0, 0.0, 1.0) );
}

mat4 inverse(mat4 m) {
    float
            a00 = m[0][0], a01 = m[0][1], a02 = m[0][2], a03 = m[0][3],
            a10 = m[1][0], a11 = m[1][1], a12 = m[1][2], a13 = m[1][3],
            a20 = m[2][0], a21 = m[2][1], a22 = m[2][2], a23 = m[2][3],
            a30 = m[3][0], a31 = m[3][1], a32 = m[3][2], a33 = m[3][3],

            b00 = a00 * a11 - a01 * a10,
            b01 = a00 * a12 - a02 * a10,
            b02 = a00 * a13 - a03 * a10,
            b03 = a01 * a12 - a02 * a11,
            b04 = a01 * a13 - a03 * a11,
            b05 = a02 * a13 - a03 * a12,
            b06 = a20 * a31 - a21 * a30,
            b07 = a20 * a32 - a22 * a30,
            b08 = a20 * a33 - a23 * a30,
            b09 = a21 * a32 - a22 * a31,
            b10 = a21 * a33 - a23 * a31,
            b11 = a22 * a33 - a23 * a32,

            det = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;

  return mat4(
                a11 * b11 - a12 * b10 + a13 * b09,
                a02 * b10 - a01 * b11 - a03 * b09,
                a31 * b05 - a32 * b04 + a33 * b03,
                a22 * b04 - a21 * b05 - a23 * b03,
                a12 * b08 - a10 * b11 - a13 * b07,
                a00 * b11 - a02 * b08 + a03 * b07,
                a32 * b02 - a30 * b05 - a33 * b01,
                a20 * b05 - a22 * b02 + a23 * b01,
                a10 * b10 - a11 * b08 + a13 * b06,
                a01 * b08 - a00 * b10 - a03 * b06,
                a30 * b04 - a31 * b02 + a33 * b00,
                a21 * b02 - a20 * b04 - a23 * b00,
                a11 * b07 - a10 * b09 - a12 * b06,
                a00 * b09 - a01 * b07 + a02 * b06,
                a31 * b01 - a30 * b03 - a32 * b00,
                a20 * b03 - a21 * b01 + a22 * b00) / det;
}

mat4 orthographic(float l, float r, float b, float t, float n, float f) {
    return mat4(
        vec4(2.0/(r-l),     0.0,          0.0,         0.0),
        vec4(0.0,           2.0/(t-b),    0.0,         0.0),
        vec4(0.0,           0.0,         -2.0/(f-n),   0.0),
        vec4(-(r+l)/(r-l), -(t+b)/(t-b), -(f+n)/(f-n), 1.0)
    );
}

mat3 lookAt(vec3 target, vec3 eye, vec3 up) {
    vec3 zaxis = normalize(target - eye);
    vec3 xaxis = normalize(cross(zaxis, up));
    vec3 yaxis = cross(zaxis, xaxis);
    return mat3(xaxis, yaxis, zaxis);
}

void main(void) {
    v_position = a_position;
    
#ifdef MODEL_VERTEX_COLOR
    v_color = a_color;
#endif
    
#ifdef MODEL_VERTEX_NORMAL
    v_normal = a_normal;
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

#ifdef LIGHT_SHADOWMAP
    v_lightCoord = vec4(0.0);//u_lightMatrix * v_position;
#endif

    mat4 V = inverse( toMat4( lookAt(vec3(0.0), -normalize(u_camera), vec3(0.0, 1.0, 0.0)) ) );
    float area = 2.0;
    mat4 P = orthographic(  area, -area, 
                            area, -area, 
                            -1.0, 5.0);

    float aspect = u_resolution.y / u_resolution.x;
    gl_Position = P * V * a_position ;
    gl_Position.xy *= vec2(aspect, 1.0) * 0.3;
    gl_Position.x -= 0.80;
    gl_Position.y += DEVLOOK_Y_OFFSET; 
})";

const std::string devlook_sphere_vert_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform vec3    u_camera;
uniform vec2    u_resolution;

in  vec4    a_position;
out vec4    v_position;

#ifdef MODEL_VERTEX_COLOR
in  vec4    a_color;
out vec4    v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
in  vec3    a_normal;
out vec3    v_normal;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
in  vec2    a_texcoord;
#endif
out vec2    v_texcoord;

#ifdef MODEL_VERTEX_TANGENT
in  vec4    a_tangent;
out vec4    v_tangent;
out mat3    v_tangentToWorld;
#endif

#ifdef LIGHT_SHADOWMAP
uniform mat4    u_lightMatrix;
out vec4    v_lightCoord;
#endif

mat4 toMat4(mat3 m) {
    return mat4(vec4(m[0], 0.0), 
                vec4(m[1], 0.0), 
                vec4(m[2], 0.0), 
                vec4(0.0, 0.0, 0.0, 1.0) );
}

mat4 inverse(mat4 m) {
    float
            a00 = m[0][0], a01 = m[0][1], a02 = m[0][2], a03 = m[0][3],
            a10 = m[1][0], a11 = m[1][1], a12 = m[1][2], a13 = m[1][3],
            a20 = m[2][0], a21 = m[2][1], a22 = m[2][2], a23 = m[2][3],
            a30 = m[3][0], a31 = m[3][1], a32 = m[3][2], a33 = m[3][3],

            b00 = a00 * a11 - a01 * a10,
            b01 = a00 * a12 - a02 * a10,
            b02 = a00 * a13 - a03 * a10,
            b03 = a01 * a12 - a02 * a11,
            b04 = a01 * a13 - a03 * a11,
            b05 = a02 * a13 - a03 * a12,
            b06 = a20 * a31 - a21 * a30,
            b07 = a20 * a32 - a22 * a30,
            b08 = a20 * a33 - a23 * a30,
            b09 = a21 * a32 - a22 * a31,
            b10 = a21 * a33 - a23 * a31,
            b11 = a22 * a33 - a23 * a32,

            det = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;

  return mat4(
                a11 * b11 - a12 * b10 + a13 * b09,
                a02 * b10 - a01 * b11 - a03 * b09,
                a31 * b05 - a32 * b04 + a33 * b03,
                a22 * b04 - a21 * b05 - a23 * b03,
                a12 * b08 - a10 * b11 - a13 * b07,
                a00 * b11 - a02 * b08 + a03 * b07,
                a32 * b02 - a30 * b05 - a33 * b01,
                a20 * b05 - a22 * b02 + a23 * b01,
                a10 * b10 - a11 * b08 + a13 * b06,
                a01 * b08 - a00 * b10 - a03 * b06,
                a30 * b04 - a31 * b02 + a33 * b00,
                a21 * b02 - a20 * b04 - a23 * b00,
                a11 * b07 - a10 * b09 - a12 * b06,
                a00 * b09 - a01 * b07 + a02 * b06,
                a31 * b01 - a30 * b03 - a32 * b00,
                a20 * b03 - a21 * b01 + a22 * b00) / det;
}

mat4 orthographic(float l, float r, float b, float t, float n, float f) {
    return mat4(
        vec4(2.0/(r-l),     0.0,          0.0,         0.0),
        vec4(0.0,           2.0/(t-b),    0.0,         0.0),
        vec4(0.0,           0.0,         -2.0/(f-n),   0.0),
        vec4(-(r+l)/(r-l), -(t+b)/(t-b), -(f+n)/(f-n), 1.0)
    );
}

mat3 lookAt(vec3 target, vec3 eye, vec3 up) {
    vec3 zaxis = normalize(target - eye);
    vec3 xaxis = normalize(cross(zaxis, up));
    vec3 yaxis = cross(zaxis, xaxis);
    return mat3(xaxis, yaxis, zaxis);
}

void main(void) {
    v_position = a_position;
    
#ifdef MODEL_VERTEX_COLOR
    v_color = a_color;
#endif
    
#ifdef MODEL_VERTEX_NORMAL
    v_normal = a_normal;
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

#ifdef LIGHT_SHADOWMAP
    v_lightCoord = vec4(0.0);//u_lightMatrix * v_position;
#endif

    mat4 V = inverse( toMat4( lookAt(vec3(0.0), -normalize(u_camera), vec3(0.0, 1.0, 0.0)) ) );
    float area = 2.0;
    mat4 P = orthographic(  area, -area, 
                            area, -area, 
                            -1.0, 5.0);

    float aspect = u_resolution.y / u_resolution.x;
    gl_Position = P * V * a_position ;
    gl_Position.xy *= vec2(aspect, 1.0) * 0.3;
    gl_Position.x -= 0.80;
    gl_Position.y += DEVLOOK_Y_OFFSET; 
})";
