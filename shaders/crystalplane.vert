#version 330
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 right;
layout(location = 2) in vec3 up;
layout(location = 3) in vec4 color;
layout(location = 4) in vec2 texcoord;

uniform mat4 u_modelViewProjectionMat;
uniform mat4 u_viewMat;
uniform mat4 u_projectionMat;
uniform float u_scale;


out vec4 v_color;

void main()
{
    vec3 pos = position;
    pos += texcoord.x * right;
    pos += texcoord.y * up;
    gl_Position = u_modelViewProjectionMat * vec4(pos, 1.0);
    v_color = color;
}

