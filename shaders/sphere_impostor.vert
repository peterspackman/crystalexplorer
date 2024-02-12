#version 330
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in float radius;
layout(location = 3) in vec2 texcoord;
layout(location = 4) in vec3 object_id;

uniform mat4 u_projectionMat;
uniform mat4 u_viewMat;
uniform mat4 u_modelViewMat;
uniform float u_scale;
uniform bool u_selection;
out vec3 v_cameraSpherePos;
out vec3 v_spherePos;
out vec2 v_mapping;
out vec4 v_color;
out float v_radius;
flat out vec3 v_object_id;
flat out int v_selected;

const float boxCorrection = 2.0;

void main()
{
    v_object_id = object_id;
    v_mapping = texcoord * boxCorrection;
    v_spherePos = position;
    v_cameraSpherePos = vec3(u_modelViewMat * vec4(position, 1.0));
    v_radius = radius * u_scale * u_scale;
    vec4 cameraCornerPos = vec4(v_cameraSpherePos, 1.0);
    cameraCornerPos.xy += clamp(texcoord, -1.0, 1.0) * boxCorrection * v_radius;
    gl_Position = u_projectionMat * cameraCornerPos; // also referred to as cameraSpherePos
    v_selected = color.x < 0 ? 1 : 0;
    v_color = abs(color);
}
