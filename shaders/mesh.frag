#version 330
#include "uniforms.glsl"

in highp vec4 v_color;
in highp vec3 v_normal;
in highp vec3 v_position;
flat in highp vec4 v_selection_id;
flat in int v_selected;
out highp vec4 f_color;

#define SELECTION_OUTLINE 1
#include "common.glsl"

void main()
{
    // Use unified shading - note the alpha is squared in original mesh shader
    float alpha = v_color.w * v_color.w;
    f_color = calculateShading(u_renderMode, v_color.xyz, u_cameraPosVec, v_position, v_normal, alpha, v_selection_id.xyz);
    f_color = applyFog(f_color, u_depthFogColor, u_depthFogOffset, u_depthFogDensity, gl_FragCoord.z);
}
