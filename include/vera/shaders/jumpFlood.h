#pragma once

#include <string>

const std::string jumpflood_encode_frag = R"(
#ifdef GL_ES
precision highp float;
#endif

uniform sampler2D   u_tex0;
uniform vec2        u_resolution;

varying vec2        v_texcoord;

#include "lygia/morphological/jumpFlood.glsl"

void main(void) {
    vec2 pixel = 1.0/u_resolution;
    vec2 st = gl_FragCoord.xy * pixel;
    vec2 uv = v_texcoord;

    // Jump flood seed
    gl_FragColor = jumpFloodEncode(u_tex0, st);
}
)";

const std::string jumpflood_frag = R"(
#ifdef GL_ES
precision highp float;
#endif

uniform sampler2D   u_floodSrc;
uniform int         u_floodIndex;
uniform int         u_floodTotal;

uniform vec2        u_resolution;
varying vec2        v_texcoord;

#include "lygia/morphological/jumpFlood.glsl"

void main() {
    vec2 pixel = 1.0/u_resolution;;
    vec2 st = gl_FragCoord.xy * pixel;
    vec2 uv = v_texcoord;
    
    st = uv;

    gl_FragColor = jumpFloodIterate(u_floodSrc, st, pixel, u_floodTotal, u_floodIndex);
}
)";

const std::string jumpflood_encode_frag_300 = R"(
#ifdef GL_ES
precision highp float;
#endif

uniform sampler2D   u_tex0;
uniform vec2        u_resolution;

in      vec2        v_texcoord;
out     vec4        fragColor;

#include "lygia/morphological/jumpFlood.glsl"

void main(void) {
    vec2 pixel = 1.0/u_resolution;
    vec2 st = gl_FragCoord.xy * pixel;
    vec2 uv = v_texcoord;

    fragColor = jumpFloodEncode(u_tex0, st);
}
)";


const std::string jumpflood_frag_300 = R"(
#ifdef GL_ES
precision highp float;
#endif

uniform sampler2D   u_floodSrc;
uniform int         u_floodIndex;
uniform int         u_floodTotal;

uniform vec2        u_resolution;

in      vec2        v_texcoord;
out     vec4        fragColor;

void main() {
    vec2 pixel = 1.0/u_resolution;
    vec2 st = gl_FragCoord.xy * pixel;
    vec2 uv = v_texcoord;

    st = uv;

    fragColor = jumpFloodIterate(u_floodSrc, st, pixel, u_floodTotal, u_floodIndex);
}
)";