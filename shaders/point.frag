#version 330
#include "uniforms.glsl"

in highp vec4 v_color;
out highp vec4 fColor;

vec3 gammaCorrection(vec3 color, float gamma) {
    return pow(color, vec3(1.0/gamma));
}

void main() {
    fColor = vec4(gammaCorrection(v_color.xyz, u_screenGamma), v_color.w);
    vec2 center = gl_PointCoord - 0.5f;
    if (dot(center, center) > 0.25f)
    {
            discard;
    }
}
