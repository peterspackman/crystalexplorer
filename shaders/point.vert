#version 330
#include "uniforms.glsl"

layout(location = 0) in highp vec3 position;
layout(location = 1) in highp vec3 color;

out highp vec4 v_color;

uniform float u_alpha;

void main()
{
  gl_PointSize = u_pointSize;
  v_color = vec4(color, u_alpha);
  gl_Position = u_modelViewProjectionMat * vec4(position, 1.0);
}
