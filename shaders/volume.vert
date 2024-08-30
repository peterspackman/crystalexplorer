#version 330 core

layout (location = 0) in vec3 position;
out vec3 v_pos;

#include "uniforms.glsl"

void main() {
    v_pos = position;
    gl_Position = u_modelViewProjectionMat * vec4(position * 5.0, 1.0);
}
