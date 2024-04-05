#version 330

layout(location = 0) in highp vec3 position;
layout(location = 1) in highp vec3 normal;
layout(location = 2) in highp vec3 color;
layout(location = 3) in highp vec3 selection_id;

out highp vec4 v_color;
out highp vec3 v_normal;
out highp vec3 v_position;
flat out highp vec4 v_selection_id;
flat out int v_selected;


uniform mat4 u_modelViewProjectionMat;
uniform float u_alpha;

void main()
{
  v_selected = 0;
  v_normal = normal;
  v_position = position;
  v_color = vec4(color, u_alpha);
  v_selection_id = vec4(selection_id, 1.0);
  gl_Position = u_modelViewProjectionMat * vec4(position, 1.0);
}
