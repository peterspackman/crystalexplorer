#version 330

#include "uniforms.glsl"

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 position;
layout(location = 2) in vec3 a;
layout(location = 3) in vec3 b;
layout(location = 4) in vec3 c;
layout(location = 5) in vec3 color;
layout(location = 6) in vec3 selectionId;

out highp vec4 v_color;
out highp vec3 v_normal;
out highp vec3 v_position;
out highp vec3 v_spherePosition;
flat out highp vec4 v_selection_id;
flat out int v_selected;
flat out int v_showLines;


void main()
{
  v_spherePosition = vertex;
  v_selected = (color.x < 0) ? 1 : 0;
  v_showLines = (color.y < 0) ? 1 : 0;
  mat4 transform = mat4(vec4(a, 0), vec4(b, 0), vec4(c, 0), vec4(position, 1));
  mat4 normalTransform = inverse(transpose(transform));
  vec4 posTransformed = transform * vec4(vertex, 1);
  v_normal = normalize(mat3(normalTransform) * vertex);
  v_position = posTransformed.xyz;
  v_color = vec4(abs(color), 1.0f);
  v_selection_id = vec4(selectionId, 1.0);
  gl_Position = u_modelViewProjectionMat * vec4(v_position, 1.0);
}

