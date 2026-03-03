#pragma once

#include <string>

const std::string fxaa_frag = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_scene;
uniform vec2        u_resolution;

#include "lygia/sample/fxaa.glsl"

void main(void) {
    vec2 pixel = 1.0 / u_resolution;
    vec2 uv = gl_FragCoord.xy * pixel;
    gl_FragColor = sampleFXAA(u_scene, uv, pixel);
})";



const std::string fxaa_frag_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D   u_scene;
uniform vec2        u_resolution;
out     vec4        fragColor;


#include "lygia/sample/fxaa.glsl"

void main(void) {
    vec2 pixel = 1.0 / u_resolution;
    vec2 uv = gl_FragCoord.xy * pixel;
    fragColor = fxaa(u_scene, uv, pixel);
})";
