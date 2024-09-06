#version 330
#include "uniforms.glsl"

in vec4 v_colorA;
in vec4 v_colorB;
in vec3 v_normal;
in vec3 v_position;
in vec3 v_cylinderPosition;
flat in vec4 v_selection_idA;
flat in vec4 v_selection_idB;
flat in int v_selectedA;
flat in int v_selectedB;
flat in int v_pattern;
out vec4 f_color;

int v_selected;

#define SELECTION_OUTLINE 1

#include "pbr.glsl"
#include "flat.glsl"

vec3 applyPattern(vec3 baseColor, float z) {
    if(v_pattern == 0) return baseColor;
    z = abs(z);
    float stripeWidth = 1.0 / 5.0;
    float pattern = step(stripeWidth * 0.5, mod(z, stripeWidth));
    return mix(baseColor, baseColor * 0.5, pattern);

    return baseColor;
}


void main()
{
    if(u_renderMode == 0) {
        if(v_cylinderPosition.z < 0) {
            f_color = v_selection_idA;
        }
        else {
            f_color = v_selection_idB;
        }
    }
    else if(u_renderMode == 1) {
        vec4 color;
        if(v_cylinderPosition.z < 0) {
            color = v_colorA;
            v_selected = v_selectedA;
        }
        else {
            color = v_colorB;
            v_selected = v_selectedB;
        }
        vec3 colorLinear = linearizeColor(color.xyz, u_screenGamma);
        colorLinear = applyPattern(colorLinear, v_cylinderPosition.z);
        colorLinear = flatWithNormalOutline(u_cameraPosVec, v_position, v_normal, colorLinear);
        f_color = vec4(unlinearizeColor(colorLinear, u_screenGamma), color.w);
        f_color = applyFog(f_color, u_depthFogColor, u_depthFogOffset, u_depthFogDensity, gl_FragCoord.z);
    }
   else {
       vec4 color;
       if(v_cylinderPosition.z < 0) {
           color = v_colorA;
           v_selected = v_selectedA;
       }
       else {
           color = v_colorB;
           v_selected = v_selectedB;
       }
       vec3 colorLinear = linearizeColor(color.xyz, u_screenGamma);
       colorLinear = applyPattern(colorLinear, v_cylinderPosition.z);

       PBRMaterial material;
       material.color = colorLinear;

       material.metallic = u_materialMetallic;
       material.roughness = u_materialRoughness;
       Lights lights;
       lights.positions = u_lightPos;
       lights.ambient = u_lightGlobalAmbient.xyz;
       lights.specular = u_lightSpecular;
       // since we're passing things through in camera space, the camera is located at the origin
       colorLinear = PBRLighting(u_cameraPosVec, v_position, v_normal, lights, material);
       f_color = vec4(unlinearizeColor(colorLinear, u_screenGamma), color.w);
       f_color = applyFog(f_color, u_depthFogColor, u_depthFogOffset, u_depthFogDensity, gl_FragCoord.z);
   }
}
