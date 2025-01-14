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

#include "flat.glsl"
#include "pbr.glsl"

vec3 applyMaskEffect(vec3 color, float maskValue) {
    float darkenFactor = 0.3;
    float edgeWidth = 0.1;
    float edgeFactor = smoothstep(1.0 - edgeWidth, 1.0, maskValue);
    return mix(color, color * darkenFactor, edgeFactor);
}

void main()
{
    float alpha = v_color.w * v_color.w;
   if(u_renderMode == 0) {
       f_color = v_selection_id;
   }
   else if(u_renderMode == 1) {
       vec4 color = v_color;
       vec3 colorLinear = linearizeColor(color.xyz, u_screenGamma);
       colorLinear = flatWithNormalOutline(u_cameraPosVec, v_position, v_normal, colorLinear);
       colorLinear = applyMaskEffect(colorLinear, v_mask);

       alpha = mix(alpha, 0.1, v_mask);
       f_color = vec4(unlinearizeColor(colorLinear, u_screenGamma), alpha);
       f_color = applyFog(f_color, u_depthFogColor, u_depthFogOffset, u_depthFogDensity, gl_FragCoord.z);
   }
   else {
       PBRMaterial material;
       material.color = linearizeColor(v_color.xyz, u_screenGamma);
       material.metallic = u_materialMetallic;
       material.roughness = u_materialRoughness;
       Lights lights;
       lights.positions = u_lightPos;
       lights.ambient = u_lightGlobalAmbient.xyz;
       lights.specular = u_lightSpecular;

       // since we're passing things through in camera space, the camera is located at the origin
       material.color = applyMaskEffect(material.color, v_mask);
       vec3 colorLinear = PBRLighting(u_cameraPosVec, v_position, v_normal, lights, material);
       alpha = mix(alpha, 0.1, v_mask);
       f_color = vec4(unlinearizeColor(colorLinear, u_screenGamma), alpha);
       f_color = applyFog(f_color, u_depthFogColor, u_depthFogOffset, u_depthFogDensity, gl_FragCoord.z);
   }
}
