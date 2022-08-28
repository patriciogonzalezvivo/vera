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

void main(void) {
    #ifdef MODEL_VERTEX_NORMAL
    gl_FragColor = vec4(normalize(u_normalMatrix * v_normal), 1.0);
    #else
    gl_FragColor = vec4(vec3(0.0), 1.0);
    #endif
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
