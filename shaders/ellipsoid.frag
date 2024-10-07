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
        float brightness = perceivedBrightness(color);
        vec3 lineColor = brightness > 0.1 ? vec3(0.01) : vec3(0.95);

        float adjustedLineWidth = 0.5 * u_ellipsoidLineWidth / u_scale;
        float featherWidth = adjustedLineWidth * 0.9; // Adjust this factor to control the smoothness
        
        if(apos.x < adjustedLineWidth) 
            color = mix(lineColor, color, smoothstep(adjustedLineWidth - featherWidth, adjustedLineWidth + featherWidth, apos.x));
        if(apos.y < adjustedLineWidth) 
            color = mix(lineColor, color, smoothstep(adjustedLineWidth - featherWidth, adjustedLineWidth + featherWidth, apos.y));
        if(apos.z < adjustedLineWidth) 
            color = mix(lineColor, color, smoothstep(adjustedLineWidth - featherWidth, adjustedLineWidth + featherWidth, apos.z));
    }
    return clamp(color, 0.0, 1.0);
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
       f_color = applyFog(f_color, u_depthFogColor, u_depthFogOffset, u_depthFogDensity, gl_FragCoord.z);
   }
   else {
       PBRMaterial material;
       material.color = linearizeColor(v_color.xyz, u_screenGamma);
       material.color = applyLines(material.color);

       material.metallic = u_materialMetallic;
       material.roughness = u_materialRoughness;
       Lights lights;
       lights.positions = u_lightPos;
       lights.ambient = u_lightGlobalAmbient.xyz;
       lights.specular = u_lightSpecular;
       // since we're passing things through in camera space, the camera is located at the origin
       vec3 colorLinear = PBRLighting(u_cameraPosVec, v_position, v_normal, lights, material);
       f_color = vec4(unlinearizeColor(colorLinear, u_screenGamma), v_color.w);
       f_color = applyFog(f_color, u_depthFogColor, u_depthFogOffset, u_depthFogDensity, gl_FragCoord.z);
   }
}
