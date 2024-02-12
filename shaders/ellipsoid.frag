#version 330
#include "uniforms.glsl"

in highp vec4 v_color;
in highp vec3 v_normal;
in highp vec3 v_position;
in highp vec3 v_spherePosition;
flat in highp vec4 v_selection_id;
flat in int v_selected;
flat in int v_showLines;
out highp vec4 f_color;

#define SELECTION_OUTLINE 1

#include "flat.glsl"
#include "pbr.glsl"

vec3 applyLines(vec3 color) {
    if(v_showLines == 1) {
        vec3 apos = abs(v_spherePosition);
        vec3 lineColor = vec3(0.05, 0.05, 0.05);
        if(apos.x < u_ellipsoidLineWidth) color =  mix(lineColor, color, smoothstep(0, u_ellipsoidLineWidth, apos.x));
        if(apos.y < u_ellipsoidLineWidth) color =  mix(lineColor, color, smoothstep(0, u_ellipsoidLineWidth, apos.y));
        if(apos.z < u_ellipsoidLineWidth) color =  mix(lineColor, color, smoothstep(0, u_ellipsoidLineWidth, apos.z));
    }
    return color;
}


void main()
{
   if(u_renderMode == 0) {
       f_color = v_selection_id;
   }
   else if(u_renderMode == 1) {
       vec4 color = v_color;
       vec3 colorLinear = linearizeColor(color.xyz, u_screenGamma);
       colorLinear = flatWithNormalOutline(u_cameraPosVec, v_position, v_normal, colorLinear);
       colorLinear = applyLines(colorLinear);
       f_color = vec4(unlinearizeColor(colorLinear, u_screenGamma), color.w);
       if(u_depthFogColor.r >= 0.0) f_color = mix(f_color, vec4(u_depthFogColor, 1.0), fogFactor(u_depthFogDensity, clamp(gl_FragCoord.z - u_depthFogOffset, 0, 1.0f)));
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
       vec3 colorLinear = PBRLighting(u_cameraPosVec, v_position, v_normal, lights, material);
       colorLinear = applyLines(colorLinear);
       f_color = vec4(unlinearizeColor(colorLinear, u_screenGamma), v_color.w);
       if(u_depthFogColor.r >= 0.0) f_color = mix(f_color, vec4(u_depthFogColor, 1.0), fogFactor(u_depthFogDensity, clamp(gl_FragCoord.z - u_depthFogOffset, 0, 1.0f)));
   }
}
