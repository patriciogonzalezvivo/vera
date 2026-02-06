#pragma once

#include <string>

const std::string dynamic_billboard_vert = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4        u_modelViewProjectionMatrix;
uniform vec2        u_translate;
uniform vec2        u_scale;
attribute vec4      a_position;
attribute vec2      a_texcoord;
varying vec2        v_texcoord;

void main(void) {
    vec4 position = a_position;
    position.xy *= u_scale;
    position.xy += u_translate;
    v_texcoord = a_texcoord;
    gl_Position = u_modelViewProjectionMatrix * position;
})";

const std::string dynamic_billboard_frag = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
uniform float       u_tex0CurrentFrame;
uniform float       u_tex0TotalFrames;

uniform vec4        u_color;
uniform vec2        u_scale;

uniform float       u_depth;
uniform float       u_area;
uniform float       u_cameraNearClip;
uniform float       u_cameraFarClip;
uniform float       u_cameraDistance;

varying vec2        v_texcoord;

#include "lygia/space/linearizeDepth.glsl"
#include "lygia/color/palette/heatmap.glsl"


void main(void) { 
    vec4 color = u_color;
    vec2 pixel = 1./u_scale;
    vec2 st = v_texcoord;

    color += texture2D(u_tex0, st);
    
    if (u_depth > 0.0) {
        float depth = color.r;
        color.rgb = heatmap( fract(linearizeDepth(depth, u_cameraNearClip, u_cameraFarClip) * 0.1) );
        color.a = 1.0;
    }

    if (u_tex0TotalFrames > 0.0)
        color.rgb = mix( color.rgb, vec3(step(st.x, u_tex0CurrentFrame/u_tex0TotalFrames), 0., 0.), step(st.y, 3. * pixel.y) );
    
    gl_FragColor = color;
}
)";

const std::string dynamic_billboard_vert_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 u_modelViewProjectionMatrix;
uniform vec2 u_translate;
uniform vec2 u_scale;
in      vec4 a_position;
in      vec2 a_texcoord;
out     vec2 v_texcoord;

void main(void) {
    vec4 position = a_position;
    position.xy *= u_scale;
    position.xy += u_translate;
    v_texcoord = a_texcoord;
    gl_Position = u_modelViewProjectionMatrix * position;
})";


const std::string dynamic_billboard_frag_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_tex0;
uniform float       u_tex0CurrentFrame;
uniform float       u_tex0TotalFrames;

uniform vec4        u_color;
uniform vec2        u_scale;

uniform float       u_depth;
uniform float       u_cameraNearClip;
uniform float       u_cameraFarClip;
uniform float       u_cameraDistance;

in      vec2        v_texcoord;
out     vec4        fragColor;

#include "lygia/space/linearizeDepth.glsl"
#include "lygia/color/palette/heatmap.glsl"

void main(void) { 
    vec4 color = u_color;
    vec2 pixel = 1./u_scale;
    vec2 st = v_texcoord;

    color += texture(u_tex0, st);
    
    if (u_depth > 0.0) {
        float depth = color.r;
        color.rgb = heatmap( fract(linearizeDepth(depth, u_cameraNearClip, u_cameraFarClip) * 0.1) );
        color.a = 1.0;
    }

    if (u_tex0TotalFrames > 0.0)
        color.rgb = mix( color.rgb, vec3(step(st.x, u_tex0CurrentFrame/u_tex0TotalFrames), 0., 0.), step(st.y, 3. * pixel.y) );
    
    fragColor = color;
}
)";
