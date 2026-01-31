#pragma once

#include <string>

// --- POINTS

const std::string points_vert = R"(#version 120
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4    u_modelViewProjectionMatrix;
uniform float   u_size;
attribute vec4  a_position;

void main(void) {
    gl_PointSize = u_size;
    gl_Position = u_modelViewProjectionMatrix * a_position;
}
)";

const std::string points_frag = R"(#version 120

#ifdef GL_OES_standard_derivatives
#extension GL_OES_standard_derivatives : enable
#endif

#ifdef GL_ES
precision mediump float;
#endif

uniform vec4 u_color;
uniform int  u_shape;

float aastep(float threshold, float value) {
    #ifdef GL_OES_standard_derivatives
    float afwidth = 0.7 * length(vec2(dFdx(value), dFdy(value)));
    return smoothstep(threshold-afwidth, threshold+afwidth, value);
    #elif defined(AA_EDGE)
    float afwidth = AA_EDGE;
    return smoothstep(threshold-afwidth, threshold+afwidth, value);
    #else 
    return step(threshold, value);
    #endif
}

#define saturate(x) clamp(x, 0.0, 1.0)

float fill(float x, float size) {
    return 1.0 - aastep(size, x);
}

float fill(float x, float size, float edge) {
    return 1.0 - smoothstep(size - edge, size + edge, x);
}

float stroke(float x, float size, float w) {
    float d = aastep(size, x + w * 0.5) - aastep(size, x - w * 0.5);
    return saturate(d);
}

float rectSDF(in vec2 st, in vec2 s) {
    st = st * 2. - 1.;
    return max( abs(st.x / s.x),
                abs(st.y / s.y) );
}

float crossSDF(in vec2 st, in float s) {
    vec2 size = vec2(.25, s);
    return min(rectSDF(st.xy, size.xy),
               rectSDF(st.xy, size.yx));
}

void main(void) {
    vec4 color = u_color;

#if !defined(GL_ES)

    if (u_shape == 1) {
        vec2 uv = gl_PointCoord.xy;
        float sdf = rectSDF(uv, vec2(1.0));
        color.a = stroke(sdf, 0.75, 0.5);
    }

    else if (u_shape == 2) {
        vec2 uv = gl_PointCoord.xy;
        float sdf = length(uv - 0.5);
        color.a = fill( sdf, 0.5 );
    }

    else if (u_shape == 3) {
        vec2 uv = gl_PointCoord.xy;
        float sdf = length(uv - 0.5) * 2.0;
        color.a = stroke( sdf, 0.75, 0.5 );
    }

    else if (u_shape == 4) {
        vec2 uv = gl_PointCoord.xy;
        color.a = fill( crossSDF(uv, 1.0), 1.0 );
    }

    else if (u_shape == 5) {
        vec2 uv = gl_PointCoord.xy;

        float sdf = .5+(uv.x-uv.y)*.5;
        float sdf_inv = (uv.x+uv.y)*.5;
        color.a =   stroke(sdf,.5,.1) + 
                    stroke(sdf_inv,.5,.1);
    }
    
#endif

    gl_FragColor = color;
}
)";

const std::string points_vert_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4    u_modelViewProjectionMatrix;
uniform float   u_size;
in      vec4    a_position;

void main(void) {
    gl_PointSize = u_size;
    gl_Position = u_modelViewProjectionMatrix * a_position;
}
)";

const std::string points_frag_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform vec4    u_color;
out     vec4    fragColor;

#ifndef SHAPE
#define SHAPE 0
#endifs

void main(void) {
    vec4 color = u_color;
    fragColor = color;
}
)";

// --- STROKE

const std::string stroke_vert = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4    u_modelViewProjectionMatrix;

uniform vec2    u_resolution;
uniform float   u_strokeWeight;

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

void main(void) {
    v_position = a_position;
    v_texcoord = a_position.xy * 0.5 + 0.5;
    
    #ifdef MODEL_VERTEX_TEXCOORD
        v_texcoord = a_texcoord;
    #endif

    #ifdef MODEL_VERTEX_COLOR
        v_color = a_color;
    #endif
    
    #ifdef MODEL_VERTEX_NORMAL
    v_normal = a_normal;
    #endif
    
    gl_Position = u_modelViewProjectionMatrix * v_position;

    #ifdef MODEL_VERTEX_NORMAL
    vec2 pixel = 1.0/u_resolution;
    gl_Position.xy += v_normal.xy * u_strokeWeight  * pixel * 100.0;
    // vec4 offset = u_modelViewProjectionMatrix * vec4(vec3(v_normal.xy, 1.0), 0.0);
    // gl_Position.xy += offset.xy * u_strokeWeight  * pixel * 100.0;
    #endif 
}
)";

const std::string stroke_frag = R"(
#ifdef GL_ES
precision mediump float;
#endif

#ifdef HAVE_TEXTURE
uniform sampler2D u_tex0;
#endif

uniform vec4    u_color;

#ifdef MODEL_VERTEX_COLOR
varying vec4    v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
varying vec3    v_normal;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
varying vec2    v_texcoord;
#endif

void main(void) {
    vec4 color = u_color;

    #ifdef MODEL_VERTEX_COLOR
    color = v_color;
    #endif

    gl_FragColor = color;
}
)";

const std::string stroke_vert_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4    u_modelViewProjectionMatrix;
in      vec4    a_position;
out     vec4    v_position;
out     vec2    v_texcoord;

#ifdef MODEL_VERTEX_NORMAL
in      vec3    a_normal;
out     vec3    v_normal;
#endif

void main(void) {
    v_texcoord = a_position.xy;
    v_position = a_position;

#ifdef MODEL_VERTEX_NORMAL
    v_normal = a_normal;
#endif

    gl_Position = u_modelViewProjectionMatrix * v_position;
}
)";

const std::string stroke_frag_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform vec4    u_color;
out     vec4    fragColor;

void main(void) {
    fragColor = u_color;
}
)";


// --- SPLINE

const std::string spline_2d_vert = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4    u_modelViewProjectionMatrix;

uniform vec2    u_resolution;
uniform float   u_strokeWeight;

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

void main(void) {
    v_position = a_position;
    v_texcoord = a_position.xy * 0.5 + 0.5;
    
    #ifdef MODEL_VERTEX_TEXCOORD
        v_texcoord = a_texcoord;
    #endif

    #ifdef MODEL_VERTEX_COLOR
        v_color = a_color;
    #endif
    
    #ifdef MODEL_VERTEX_NORMAL
    v_normal = a_normal;
    #endif
    
    gl_Position = u_modelViewProjectionMatrix * v_position;

    #ifdef MODEL_VERTEX_NORMAL
    vec2 pixel = 1.0/u_resolution;
    gl_Position.xy += v_normal.xy * u_strokeWeight  * pixel * 100.0;

    // vec4 offset = u_modelViewProjectionMatrix * vec4(vec3(v_normal.xy, 1.0), 0.0);
    // gl_Position.xy += offset.xy * u_strokeWeight  * pixel * 100.0;
    #endif 
}
)";

const std::string spline_2d_frag = R"(
#ifdef GL_ES
precision mediump float;
#endif

#ifdef HAVE_TEXTURE
uniform sampler2D u_tex0;
#endif

uniform vec4    u_color;

#ifdef MODEL_VERTEX_COLOR
varying vec4    v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
varying vec3    v_normal;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
varying vec2    v_texcoord;
#endif

void main(void) {
    vec4 color = u_color;

    #ifdef MODEL_VERTEX_COLOR
    color = v_color;
    #endif

    gl_FragColor = color;
}
)";

const std::string spline_2d_vert_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4    u_modelViewProjectionMatrix;
in      vec4    a_position;
out     vec4    v_position;
out     vec2    v_texcoord;

#ifdef MODEL_VERTEX_NORMAL
in      vec3    a_normal;
out     vec3    v_normal;
#endif

void main(void) {
    v_texcoord = a_position.xy;
    v_position = a_position;

#ifdef MODEL_VERTEX_NORMAL
    v_normal = a_normal;
#endif

    gl_Position = u_modelViewProjectionMatrix * v_position;
}
)";

const std::string spline_2d_frag_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform vec4    u_color;
out     vec4    fragColor;

void main(void) {
    fragColor = u_color;
}
)";


const std::string spline_3d_vert = R"(
#ifdef GL_ES
precision mediump float;
#endif


uniform mat4    u_viewMatrix;
uniform mat4    u_modelMatrix;
uniform mat4    u_modelViewMatrix;
uniform mat4    u_modelViewProjectionMatrix;

uniform vec4    u_color;
uniform vec2    u_resolution;
uniform vec3    u_camera;
uniform float   u_cameraDistance;

uniform float   u_strokeWeight;

attribute vec4  a_position;
attribute vec4  a_color;
attribute vec3  a_normal;
attribute vec2  a_texcoord;

varying vec4    v_position;
varying vec2    v_texcoord;

const float u_scale = 1.0;

vec2 fix(vec4 i) {
    vec2 res = i.xy / i.w;
    return res;
}

vec3 extrudeNormal(vec2 p0, vec2 p1) {
    vec2 dir = normalize(p1 - p0);
    return vec3(-dir.y, dir.x, 0.0);
}

void main(void) {
    
    vec4 position_prev = a_color;
    vec4 position_curr = a_position;
    vec4 position_next = vec4(a_normal.xyz, 1.0);

    v_texcoord = a_texcoord;

    vec2 pixel = 1.0 / u_resolution;
    float aspect = u_resolution.x / u_resolution.y;
    float side = v_texcoord.x * 2.0 - 1.0;
    float width = (u_strokeWeight + u_cameraDistance) * 5.0 * side;

    // into clip space
    position_prev = u_modelViewProjectionMatrix * position_prev;
    position_curr = u_modelViewProjectionMatrix * position_curr;
    position_next = u_modelViewProjectionMatrix * position_next;

    // into NDC space [-1 .. 1]
    vec2 screen_pos_prev = fix(position_prev);
    vec2 screen_pos_curr = fix(position_curr);
    vec2 screen_pos_next = fix(position_next);

    // calculate the extrude normals
    vec3 n1 = extrudeNormal(screen_pos_prev, screen_pos_curr);
    vec3 n2 = extrudeNormal(screen_pos_curr, screen_pos_next);

    position_curr.xy += normalize(n1.xy + n2.xy) * width * pixel;
    // position_curr.xy += (n1.xy + n2.xy) / (1.0 + dot(n1.xy, n2.xy)) * width * pixel;

    gl_Position = position_curr;
}
)";

const std::string spline_3d_frag = R"(
#ifdef GL_ES
precision mediump float;
#endif

#ifdef HAVE_TEXTURE
uniform sampler2D u_tex0;
#endif

uniform vec4    u_color;

varying vec4    v_position;
varying vec2    v_texcoord;

void main(void) {
    vec4 color = u_color;
    gl_FragColor = color;
}
)";

const std::string spline_3d_vert_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4    u_modelViewProjectionMatrix;
in      vec4    a_position;
out     vec4    v_position;
out     vec2    v_texcoord;

#ifdef MODEL_VERTEX_NORMAL
in      vec3    a_normal;
out     vec3    v_normal;
#endif

void main(void) {
    v_texcoord = a_position.xy;
    v_position = a_position;

#ifdef MODEL_VERTEX_NORMAL
    v_normal = a_normal;
#endif

    gl_Position = u_modelViewProjectionMatrix * v_position;
}
)";

const std::string spline_3d_frag_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform vec4    u_color;
out     vec4    fragColor;

void main(void) {
    fragColor = u_color;
}
)";


// --- FILL

const std::string fill_vert = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4    u_modelViewProjectionMatrix;

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

void main(void) {
    v_position = a_position;
    v_texcoord = a_position.xy * 0.5 + 0.5;
    
#ifdef MODEL_VERTEX_COLOR
    v_color = a_color;
#endif
    
#ifdef MODEL_VERTEX_NORMAL
    v_normal = a_normal;
#endif
    
#ifdef MODEL_VERTEX_TEXCOORD
    v_texcoord = a_texcoord;
#endif
    
    gl_Position = u_modelViewProjectionMatrix * v_position;
}
)";

const std::string fill_frag = R"(
#ifdef GL_ES
precision mediump float;
#endif

#ifdef HAVE_TEXTURE
uniform sampler2D u_tex0;
#endif

uniform vec4    u_color;

#ifdef MODEL_VERTEX_COLOR
varying vec4    v_color;
#endif

#ifdef MODEL_VERTEX_NORMAL
varying vec3    v_normal;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
varying vec2    v_texcoord;
#endif

void main(void) {
    vec4 color = u_color;

#ifdef MODEL_VERTEX_COLOR
    color = v_color;
#endif

#if defined(HAVE_TEXTURE) && defined(MODEL_VERTEX_TEXCOORD)
    color = texture2D(u_tex0, v_texcoord);
#endif

    gl_FragColor = color;
}
)";

const std::string fill_vert_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4    u_modelViewProjectionMatrix;
in      vec4    a_position;
out     vec2    v_texcoord;

void main(void) {
    v_texcoord = a_position.xy;
    gl_Position = u_modelViewProjectionMatrix * a_position;
}
)";

const std::string fill_frag_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform vec4    u_color;
out     vec4    fragColor;

void main(void) {
    fragColor = u_color;
}
)";

