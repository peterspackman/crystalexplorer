#version 330

layout(location = 0) in highp vec3 position;
layout(location = 1) in highp vec3 normal;
layout(location = 2) in highp vec3 translation;
layout(location = 3) in highp mat3 rotation;
layout(location = 6) in highp vec3 selection_id;
layout(location = 7) in highp float alpha;

out highp vec4 v_color;
out highp vec3 v_normal;
out highp vec3 v_position;
flat out highp vec4 v_selection_id;
flat out int v_selected;


uniform mat4 u_modelViewProjectionMat;

void main()
{
  v_selected = 0;
  v_normal = rotation * normal;
  v_position = rotation * position + translation;
  v_color = vec4(selection_id, alpha);
  v_selection_id = vec4(selection_id, 1.0);
  gl_Position = u_modelViewProjectionMat * vec4(v_position, 1.0);
}
