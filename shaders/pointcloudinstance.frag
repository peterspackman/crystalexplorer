#version 330
#include "uniforms.glsl"
in highp vec4 v_color;
in highp vec3 v_normal;
in highp vec3 v_position;
flat in highp vec4 v_selection_id;
flat in int v_selected;
out highp vec4 f_color;

#define SELECTION_OUTLINE 1

#include "flat.glsl"
#include "pbr.glsl"



vec3 gammaCorrection(vec3 color, float gamma) {
    return pow(color, vec3(1.0/gamma));
}

void main() {
    f_color.xyz = v_color.xyz;
    f_color.w = v_color.w * v_color.w;
    f_color = applyFog(f_color, u_depthFogColor, u_depthFogOffset, u_depthFogDensity, gl_FragCoord.z);

    vec2 center = gl_PointCoord - 0.5f;
    if (dot(center, center) > 0.25f)
    {
            discard;
    }
}
