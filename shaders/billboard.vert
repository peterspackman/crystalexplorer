#version 330

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 dimensions;
layout(location = 2) in float alpha;
layout(location = 3) in vec2 texcoord;

uniform mat4 u_projectionMat;
uniform mat4 u_viewMat;
uniform float u_scale;

out vec2 v_texcoord;
out float v_alpha;

void main()
{
    v_texcoord = texcoord;
    v_alpha = alpha;

    vec3 cameraPos = vec3(u_viewMat * vec4(position, 1.0));
    cameraPos.z += 0.5 * u_scale;

    vec4 cameraCornerPos = vec4(cameraPos, 1.0);
    cameraCornerPos.xy += (0.002 * texcoord * dimensions) * clamp(u_scale * u_scale, 0.5, 10.0);

    gl_Position = u_projectionMat * cameraCornerPos;
}
