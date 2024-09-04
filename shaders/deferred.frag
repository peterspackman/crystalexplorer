#version 330

in vec2 TexCoords;
out vec4 FragColor;

// G-Buffer textures
uniform sampler2D g_position;
uniform sampler2D g_material;
uniform sampler2D g_normal;
uniform sampler2D g_albedo;

int v_selected = 0;

#include "uniforms.glsl"
#include "pbr.glsl"

void main()
{
    // Retrieve data from G-buffer
    vec3 position = texture(g_position, TexCoords).rgb;
    vec3 normal = texture(g_normal, TexCoords).rgb;
    vec4 albedo = texture(g_albedo, TexCoords);
    vec4 material = texture(g_material, TexCoords);
	FragColor = albedo;
	return;

    // Construct PBRMaterial
    PBRMaterial mat;
    mat.color = albedo.rgb;
    mat.metallic = u_materialMetallic;
    mat.roughness = u_materialRoughness;

    Lights lights;
    lights.positions = u_lightPos;
    lights.ambient = u_lightGlobalAmbient.xyz;
    lights.specular = u_lightSpecular;

    v_selected = int(material.b);

    // Perform PBR lighting
    vec3 colorLinear = PBRLighting(u_cameraPosVec, position, normal, lights, mat);

    FragColor = vec4(unlinearizeColor(colorLinear, u_screenGamma), albedo.w);
    FragColor = applyFog(FragColor, u_depthFogColor, u_depthFogOffset, u_depthFogDensity, material.g);

}
