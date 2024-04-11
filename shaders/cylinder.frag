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
out vec4 f_color;

int v_selected;


// Sphere occlusion
float sphOcclusion( in vec3 pos, in vec3 nor, in vec4 sph )
{
    vec3  di = sph.xyz - pos;
    float l  = length(di);
    float nl = dot(nor,di/l);
    float h  = l/sph.w;
    float h2 = h*h;
    float k2 = 1.0 - h2*nl*nl;

    // above/below horizon
    // EXACT: Quilez - https://iquilezles.org/articles/sphereao
    float res = max(0.0,nl)/h2;

    // intersecting horizon
    if( k2 > 0.0 )
    {
        // APPROXIMATED : Quilez - https://iquilezles.org/articles/sphereao
        res = (nl*h+1.0)/h2;
        res = 0.33*res*res;
    }
    return res;
}


#define SELECTION_OUTLINE 1

#include "pbr.glsl"
#include "flat.glsl"


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
        colorLinear = flatWithNormalOutline(u_cameraPosVec, v_position, v_normal, colorLinear);
        f_color = vec4(unlinearizeColor(colorLinear, u_screenGamma), color.w);
        if(u_depthFogColor.r >= 0.0) f_color = mix(f_color, vec4(u_depthFogColor, 1.0), fogFactor(u_depthFogDensity, clamp(gl_FragCoord.z - u_depthFogOffset, 0, 1.0f)));
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
       if(u_depthFogColor.r >= 0.0) f_color = mix(f_color, vec4(u_depthFogColor, 1.0), fogFactor(u_depthFogDensity, clamp(gl_FragCoord.z - u_depthFogOffset, 0, 1.0f)));
   }
}
