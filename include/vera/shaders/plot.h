#pragma once

#include <string>

const std::string plot_frag = R"(
#ifdef GL_ES
precision mediump float;
#endif

#ifndef PLOT_DATA_TEXTURE
#define PLOT_DATA_TEXTURE u_plotData
#endif

#ifndef PLOT_DATA_PIXEL
#define PLOT_DATA_PIXEL 0.00390625
#endif

uniform sampler2D PLOT_DATA_TEXTURE;
uniform vec2 u_viewport;

varying vec2 v_texcoord;

#include "lygia/space/ratio.glsl"
#include "lygia/space/scale.glsl"
#include "lygia/draw/stroke.glsl"
#include "lygia/draw/flip.glsl"
#include "lygia/draw/digits.glsl"

void main() {
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    vec2 st = v_texcoord;

    vec4 data = texture2D(PLOT_DATA_TEXTURE, vec2(st.x, 0.5));

    color.r = step(st.y, data.r);
    color.g = step(st.y, data.g);
    color.b = step(st.y, data.b);

    #ifdef PLOT_VALUE
    vec4 value = texture2D(PLOT_DATA_TEXTURE, vec2(1.0-PLOT_DATA_PIXEL*0.5, 0.5));
    vec2 uv = ratio(st, u_viewport);
    PLOT_VALUE
    #endif

    gl_FragColor = color;
}
)";

const std::string plot_frag_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

#ifndef PLOT_DATA_TEXTURE
#define PLOT_DATA_TEXTURE u_plotData
#endif

uniform sampler2D   PLOT_DATA_TEXTURE;

in      vec2        v_texcoord;
out     vec4        fragColor;

#include "lygia/space/ratio.glsl"
#include "lygia/space/scale.glsl"
#include "lygia/draw/stroke.glsl"
#include "lygia/draw/flip.glsl"
#include "lygia/draw/digits.glsl"

void main() {
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    vec2 st = v_texcoord;

    vec4 data = texture(PLOT_DATA_TEXTURE, vec2(st.x, 0.5));

    color.r = step(st.y, data.r);
    color.g = step(st.y, data.g);
    color.b = step(st.y, data.b);

    #ifdef PLOT_VALUE
    vec4 value = texture(PLOT_DATA_TEXTURE, vec2(1.0-PLOT_DATA_PIXEL*0.5, 0.5));
    vec2 uv = ratio(st, u_viewport);
    PLOT_VALUE
    #endif

    fragColor = color;
}
)";
