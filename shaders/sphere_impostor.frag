#version 330

in vec2 v_mapping;
in vec4 v_color;
in vec3 v_cameraSpherePos;
in vec3 v_spherePos;
in float v_radius;
flat in int v_selected;
flat in vec3 v_object_id;
out vec4 outputColor;

uniform mat4 u_viewMat;
uniform mat4 u_normalMat;
uniform mat4 u_modelViewMatInv;
uniform vec3 u_cameraPosVec;
uniform mat4 u_projectionMat;
uniform mat4 u_lightPos;
uniform vec4 u_lightDiffuse;
uniform mat4 u_lightSpecular;
uniform vec4 u_lightGlobalAmbient;
uniform vec2 u_attenuationClamp;
uniform vec4 u_selectionColor;
uniform bool u_selectionMode;
uniform float u_screenGamma;
uniform float u_materialRoughness;
uniform float u_materialMetallic;
uniform int u_numLights;

#define SELECTION_OUTLINE 1
/*
  COPIED FROM pbr.glsl, don't modify directly
  */


struct PBRMaterial {
    vec3 color;
    float metallic;
    float roughness;

};

struct Lights {
    mat4 positions;
    vec3 ambient;
    mat4 specular;
};

const float M_PI = 3.14159265359;

float DistributionGGX(float NdotH, float roughness)
{
    float a2 = roughness * roughness;
    float f = (NdotH * a2 - NdotH) * NdotH + 1.0;
    return a2 / (M_PI * f * f);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}
float GeometrySmith(float NdotL, float NdotV, float roughness)
{
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}


vec3 PBRLighting(in vec3 cameraPosition, in vec3 position, in vec3 normal, in Lights lights, in PBRMaterial material) {
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, material.color, material.metallic);
    vec3 Lo = vec3(0.0);

    vec3 V = normalize(cameraPosition - position);
    vec3 N = normalize(normal);
    float ao = 1.0;

    // for when we have more than one light
    for(int i = 0; i < u_numLights; i++) {
        vec3 L = normalize(lights.positions[i].xyz - position);
        float distance = length(lights.positions[i].xyz - position);
        float attenuation = 2.0;
        vec3 radiance = lights.specular[i].rgb * attenuation;
        vec3 H = normalize(V + L);
        float NdotV = abs(dot(N, V)) + 1e-5;
        float NdotL = clamp(dot(N, L), 0.0, 1.0);
        float NdotH = clamp(dot(N, H), 0.0, 1.0);
        float LdotH = clamp(dot(L, H), 0.0, 1.0);
        // Cook-Torrance BRDF
        float D = DistributionGGX(NdotH, material.roughness);
        float G = GeometrySmith(NdotL, NdotV, material.roughness);
        vec3 F = FresnelSchlick(LdotH, F0);

        vec3 numerator    = D * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.001) * max(dot(N, L), 0.0) + 0.00000000001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;

        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - material.metallic;

        // scale light by NdotL
        // add to outgoing radiance Lo
        Lo += (kD * material.color / M_PI + specular) * radiance * NdotL;
    }
    vec3 ambient = lights.ambient * material.color * ao;
    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));
#ifdef SELECTION_OUTLINE
    if(v_selected == 1) {
        float c = dot(N, V);
        // could use c to mix as well
        color = mix(vec3(u_selectionColor), color, smoothstep(0.0, 0.5, c*c));
    }
#endif

    return color;
}


/*
  END pbr.glsl section
  */



float calcDepth( in vec3 cameraPos ){
    vec2 clipZW = cameraPos.z * u_projectionMat[2].zw + u_projectionMat[3].zw;
    return 0.5 + 0.5 * clipZW.x / clipZW.y;
}


float intersectCorner(out vec3 cameraPos, out vec3 cameraNormal, vec2 mapping)
{
    vec3 cameraPlanePos = vec3(mapping * v_radius, 0.0) + v_cameraSpherePos;
    vec3 rayDirection = normalize(cameraPlanePos);

    float b = 2.0 * dot(rayDirection, -v_cameraSpherePos);
    float c = dot(v_cameraSpherePos, v_cameraSpherePos) - (v_radius * v_radius);

    float h = (b * b) - (4 * c);
    if(h < 0.0) {
        return h;
    }
    h = sqrt(h);
    float plus = (-b + h)/2;
    float minus = (-b - h)/2;

    // pick the first intersection
    float intersectionDepth = min(plus, minus);

    cameraPos = rayDirection * intersectionDepth;
    cameraNormal = normalize(cameraPos - v_cameraSpherePos);
    return h;
}

float sphereIntersect(out vec3 cameraPos, out vec3 cameraNormal)
{
    cameraPos = vec3(0);
    cameraNormal = vec3(0);
#ifdef USE_MULTISAMPLE

    vec3 pos, norm;
    int numInside = 0;

    vec2 dx = dFdx(v_mapping);
    vec2 dy = dFdy(v_mapping);
    float h1, h2, h3, h4;
    if(intersectCorner(pos, norm, v_mapping - -dx - dy) >= 0) {
        cameraPos += pos;
        cameraNormal += norm;
        numInside++;
    }
    if(intersectCorner(pos, norm, v_mapping - dx + dy) >= 0) {
        cameraPos += pos;
        cameraNormal += norm;
        numInside++;
    }
    if(intersectCorner(pos, norm, v_mapping + dx - dy) >= 0) {
        cameraPos += pos;
        cameraNormal += norm;
        numInside++;
    }
    if(intersectCorner(pos, norm, v_mapping + dx + dy) >= 0) {
        cameraPos += pos;
        cameraNormal += norm;
        numInside++;
    }
    if(numInside <= 0) discard;
    cameraPos /= numInside;
    cameraNormal /= numInside;
    return numInside / 4.0;
#else
    if(intersectCorner(cameraPos, cameraNormal, v_mapping) < 0.0) discard;
    return 1.0;
#endif

}

void main()
{
    vec3 cameraPos;
    vec3 cameraNormal;

    float coverage = sphereIntersect(cameraPos, cameraNormal);

    gl_FragDepth = calcDepth(cameraPos);
    if(u_selectionMode) {
        outputColor = vec4(v_object_id, 1.0f);
    }
    else {
        // if we need the world space normal
        // cameraNormal = mat3(transpose(u_viewMat)) * cameraNormal;

        PBRMaterial material;
        material.color = v_color.xyz;
        material.metallic = u_materialMetallic;
        material.roughness = u_materialRoughness;
        Lights lights;
        lights.positions = u_lightPos;
        lights.ambient = u_lightGlobalAmbient.xyz;
        lights.specular = u_lightSpecular;
        vec4 worldSpacePos = u_modelViewMatInv * vec4(cameraPos, 1.0f);
        vec4 worldSpaceNormal = transpose(u_viewMat) * vec4(cameraNormal, 1.0f);

        // since we're passing things through in camera space, the camera is located at the origin
        vec3 colorLinear = PBRLighting(u_cameraPosVec, worldSpacePos.xyz, worldSpaceNormal.xyz, lights, material);
        outputColor = vec4(pow(colorLinear, vec3(1.0 / u_screenGamma)), coverage * v_color.w*v_color.w);
//        vec4 colorLinear = Lighting(cameraPos, cameraNormal);
//        vec3 colorGammaCorrected = pow(colorLinear.xyz, vec3(1.0/u_screenGamma));
//        outputColor = vec4(colorGammaCorrected, 1.0f);
    }
}
