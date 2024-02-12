#version 330

layout(location = 10) in highp vec3 position;
layout(location = 11) in highp vec3 normal;
layout(location = 12) in highp float color;
out highp vec4 v_color;
out highp vec3 v_normal;
out highp vec3 v_position;

uniform mat4 u_MVPMat;
uniform mat3 u_colorscheme;
uniform float u_minColorValue;
uniform float u_maxColorValue;

vec4 colormap(float value, float alpha) {
    int index = 0;
    float factor = 0;
    if (value < 0) {
        factor = 1.0 - value / u_minColorValue;
        index = 2;
    }
    else factor = 1.0 - value / u_maxColorValue;

    vec3 color = u_colorscheme[index];
    vec3 mid = u_colorscheme[1];
    vec3 result = color + (mid - color) * factor;
    return vec4(result, alpha);
}


void main()
{
  v_normal = normal;
  v_position = position;
  gl_Position = u_MVPMat * vec4(position, 1.0);
  v_color = colormap(color, 1.0);
}
