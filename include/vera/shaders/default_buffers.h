#pragma once

#include <string>

static const std::string default_buffer_position = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4        u_viewMatrix;
uniform mat4        u_modelMatrix;
varying vec4        v_position;

void main(void) {
    gl_FragColor = u_viewMatrix * u_modelMatrix * v_position;
}
)";

static const std::string default_buffer_position_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat4        u_viewMatrix;
uniform mat4        u_modelMatrix;
in      vec4        v_position;
out     vec4        fragColor;

void main(void) {
    fragColor = u_viewMatrix * u_modelMatrix * v_position;
}
)";

static const std::string default_buffer_normal = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat3        u_normalMatrix;

#ifdef MODEL_VERTEX_NORMAL
varying vec3        v_normal;
#endif

#ifdef MODEL_VERTEX_TEXCOORD
varying vec2        v_texcoord;
#endif

#ifdef MODEL_VERTEX_TANGENT
varying vec4        v_tangent;
varying mat3        v_tangentToWorld;
#endif

#ifndef FNC_MATERIAL_NORMAL
#define FNC_MATERIAL_NORMAL

#ifdef MATERIAL_NORMALMAP
uniform sampler2D MATERIAL_NORMALMAP;
#endif

#ifdef MATERIAL_BUMPMAP_NORMALMAP
uniform sampler2D MATERIAL_BUMPMAP_NORMALMAP;
#endif

vec3 materialNormal() {
    vec3 normal = vec3(0.0, 0.0, 1.0);

#ifdef MODEL_VERTEX_NORMAL
    normal = v_normal;

    #if defined(MODEL_VERTEX_TANGENT) && defined(MODEL_VERTEX_TEXCOORD) && defined(MATERIAL_NORMALMAP) 
    vec2 uv = v_texcoord.xy;
        #if defined(MATERIAL_NORMALMAP_OFFSET)
    uv += (MATERIAL_NORMALMAP_OFFSET).xy;
        #endif
        #if defined(MATERIAL_NORMALMAP_SCALE)
    uv *= (MATERIAL_NORMALMAP_SCALE).xy;
        #endif
    normal = texture2D(MATERIAL_NORMALMAP, uv).xyz;
    normal = v_tangentToWorld * (normal * 2.0 - 1.0);

    #elif defined(MODEL_VERTEX_TANGENT) && defined(MODEL_VERTEX_TEXCOORD) && defined(MATERIAL_BUMPMAP_NORMALMAP)
    vec2 uv = v_texcoord.xy;
        #if defined(MATERIAL_BUMPMAP_OFFSET)
    uv += (MATERIAL_BUMPMAP_OFFSET).xy;
        #endif
        #if defined(MATERIAL_BUMPMAP_SCALE)
    uv *= (MATERIAL_BUMPMAP_SCALE).xy;
        #endif
    normal = v_tangentToWorld * (texture2D(MATERIAL_BUMPMAP_NORMALMAP, uv).xyz * 2.0 - 1.0);
    #endif
    
#endif

    return normal;
}
#endif


void main(void) {
    vec3 normal = materialNormal();
    gl_FragColor = vec4(normalize(u_normalMatrix * normal), 1.0);
}
)";

static const std::string default_buffer_normal_300 = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform mat3        u_normalMatrix;
#ifdef MODEL_VERTEX_NORMAL
in      vec3        v_normal;
#endif
out     vec4        fragColor;

void main(void) {
    #ifdef MODEL_VERTEX_NORMAL
    fragColor = vec4(normalize(u_normalMatrix * v_normal), 0.0);
    #else
    fragColor = vec4(0.0);
    #endif
}
)";
