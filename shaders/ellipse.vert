#version 330
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 right;
layout(location = 2) in vec3 up;
layout(location = 3) in vec4 color;
layout(location = 4) in vec2 texcoord;
layout(location = 5) in float r1squared;
layout(location = 6) in float max_angle;

uniform mat4 u_modelViewProjectionMat;
uniform mat4 u_viewMat;
uniform mat4 u_projectionMat;
uniform float u_scale;


out vec2 v_mapping;
out vec4 v_color;
flat out float v_max_angle;
flat out float v_r1squared;

void main()
{
    v_r1squared = r1squared;
    v_max_angle = max_angle;
    v_mapping = texcoord;
    vec3 pos = position;
    pos += texcoord.x * right;
    pos += texcoord.y * up;
    gl_Position = u_modelViewProjectionMat * vec4(pos, 1.0);
    v_color = color;
}
