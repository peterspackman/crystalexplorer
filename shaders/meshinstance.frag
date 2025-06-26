#version 330
#include "uniforms.glsl"

in highp vec4 v_color;
in highp vec3 v_normal;
in highp vec3 v_position;
flat in highp vec4 v_selection_id;
flat in int v_selected;
in float v_mask;
out highp vec4 f_color;

#define SELECTION_OUTLINE 1
#include "common.glsl"

vec3 applyMaskEffect(vec3 color, float maskValue) {
    float darkenFactor = 0.3;
    float edgeWidth = 0.1;
    float edgeFactor = smoothstep(1.0 - edgeWidth, 1.0, maskValue);
    return mix(color, color * darkenFactor, edgeFactor);
}

void main()
{
    // Apply mask effect to base color and calculate alpha
    vec3 materialColor = applyMaskEffect(v_color.xyz, v_mask);
    float alpha = v_color.w * v_color.w;
    alpha = mix(alpha, 0.1, v_mask);
    
    // Use unified shading
    f_color = calculateShading(u_renderMode, materialColor, u_cameraPosVec, v_position, v_normal, alpha, v_selection_id.xyz);
    f_color = applyFog(f_color, u_depthFogColor, u_depthFogOffset, u_depthFogDensity, gl_FragCoord.z);
}
