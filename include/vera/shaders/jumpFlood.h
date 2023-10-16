#pragma once

#include <string>

const std::string jumpflood_frag = R"(
#ifdef GL_ES
precision highp float;
#endif

uniform sampler2D   u_floodSrc;
uniform int         u_floodIndex;
uniform int         u_floodTotal;

uniform vec2        u_resolution;

varying vec2 v_texcoord;

void main() {
    vec4 color = vec4(0.0);
    vec2 pixel = 1.0/u_resolution;
    vec2 st = gl_FragCoord.xy * pixel;
    // vec2 st = v_texcoord;

    float jDist = pow(2.0, float(u_floodTotal - u_floodIndex - 1) );

    vec2 offsets[9];
    offsets[0] = vec2(-1.0, -1.0);
    offsets[1] = vec2(-1.0, 0.0);
    offsets[2] = vec2(-1.0, 1.0);
    offsets[3] = vec2(0.0, -1.0);
    offsets[4] = vec2(0.0, 0.0);
    offsets[5] = vec2(0.0, 1.0);
    offsets[6] = vec2(1.0, -1.0);
    offsets[7] = vec2(1.0, 0.0);
    offsets[8] = vec2(1.0, 1.0);
    
    float closest_dist = 9999999.9;
    vec2 closest_pos = vec2(0.0);
    vec4 closest_data = vec4(0.0);

    for(int i = 0; i < 9; i++) {
        vec2 jump = st + (offsets[i] * jDist * pixel);
        vec4 seed = texture2D(u_floodSrc, jump);
        vec2 seedpos = seed.xy;
        float dist = distance(seedpos, st);
        if (seedpos != vec2(0.0) && dist <= closest_dist) {
            closest_dist = dist;
            closest_data = seed;
        }
    }

    gl_FragColor = closest_data;
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
    vec4 color = vec4(0.0);
    vec2 pixel = 1.0/u_resolution;
    vec2 st = gl_FragCoord.xy * pixel;
    // vec2 st = v_texcoord;

    float jDist = pow(2.0, flood(u_floodTotal - u_floodIndex - 1));

    vec2 offsets[9];
    offsets[0] = vec2(-1.0, -1.0);
    offsets[1] = vec2(-1.0, 0.0);
    offsets[2] = vec2(-1.0, 1.0);
    offsets[3] = vec2(0.0, -1.0);
    offsets[4] = vec2(0.0, 0.0);
    offsets[5] = vec2(0.0, 1.0);
    offsets[6] = vec2(1.0, -1.0);
    offsets[7] = vec2(1.0, 0.0);
    offsets[8] = vec2(1.0, 1.0);
    
    float closest_dist = 9999999.9;
    vec2 closest_pos = vec2(0.0);
    vec4 closest_data = vec4(0.0);

    for(int i = 0; i < 9; i++) {
        vec2 jump = st + (offsets[i] * jDist * pixel);
        vec4 seed = texture2D(u_floodSrc, jump);
        vec2 seedpos = seed.xy;
        float dist = distance(seedpos, st);
        if (seedpos != vec2(0.0) && dist <= closest_dist) {
            closest_dist = dist;
            closest_data = seed;
        }
    }

    fragColor = closest_data;
}
)";