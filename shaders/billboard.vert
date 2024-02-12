#version 330
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 dimensions;
layout(location = 2) in float alpha;
layout(location = 3) in vec2 texcoord;

uniform mat4 u_projectionMat;
uniform mat4 u_viewMat;
uniform float u_scale;
out vec3 v_cameraPos;
out vec2 v_mapping;


void main()
{
    v_mapping = texcoord;
    v_cameraPos = vec3(u_viewMat * vec4(position, 1.0));
    v_cameraPos.z += 0.5 * u_scale;
    vec4 cameraCornerPos = vec4(v_cameraPos, 1.0);
    cameraCornerPos.xy += (0.002 * texcoord * dimensions) * clamp(u_scale * u_scale, 0.5, 10.0);
    gl_Position = u_projectionMat * cameraCornerPos;
}
