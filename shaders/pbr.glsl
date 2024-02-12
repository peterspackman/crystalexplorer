
struct PBRMaterial {
    vec3 color;
    float metallic;
    float roughness;
    float clearcoat;
    float clearcoatRoughness;
};

struct Lights {
    mat4 positions;
    vec3 ambient;
    mat4 specular;
};

vec3 linearizeColor(vec3 color, float gamma) {
    return pow(color, vec3(gamma));
}

vec3 unlinearizeColor(vec3 color, float gamma) {
    return pow(color, 1.0 / vec3(gamma));
}

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

vec3 Uncharted2ToneMap(vec3 x)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;

    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 ACESFilmToneMap(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

vec3 ReinhardToneMap(vec3 x)
{
    return x / (x + vec3(1.0));
}


vec3 ToneMap(int mapId, vec3 x)
{
    switch(mapId) {
    default:
        return ReinhardToneMap(x);
    case 1:
        return ACESFilmToneMap(x);
    case 2:
        return Uncharted2ToneMap(x);
    }
}

vec3 PBRLighting(in vec3 cameraPosition, in vec3 position, in vec3 normal, in Lights lights, in PBRMaterial material) {
    material.clearcoat = 0.0;
    material.clearcoatRoughness = 0.05;
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
        float attenuation = clamp(1.0f/ (1.0f + distance * distance), u_attenuationClamp.x, u_attenuationClamp.y);
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

        // clearcoat
        float Dr = DistributionGGX(NdotH, material.clearcoatRoughness);
        vec3 Fr = FresnelSchlick(max(dot(N, V), 0.0), vec3(0.04));
        float Gr = GeometrySmith(NdotL, NdotV, material.clearcoatRoughness);
        vec3 clearcoatBRDF = vec3(0.25) * Dr * Fr * Gr;

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
        // the factor of 0.25 in clearcoatBRDF comes from the Fresnel term for dielectrics which averages to 0.04 or 4% reflectance.
        // This also conveniently scales our clearcoat layer to 25% of the total reflection which is a common approximation.
        Lo += (kD * material.color / M_PI + specular + material.clearcoat * clearcoatBRDF) * radiance * NdotL;
    }
    vec3 ambient = lights.ambient * material.color * ao;
    vec3 color = ambient + Lo;
    color *= u_lightingExposure; // exposure
    color = ToneMap(u_toneMapIdentifier, color);
#ifdef SELECTION_OUTLINE
    if(v_selected == 1) {
        float c = dot(N, V);
        // could use c to mix as well
        color = mix(vec3(u_selectionColor), color, smoothstep(0.0, 0.5, c*c));
    }
#endif

    return color;
}

float fogFactor(float density, float fc) {
    float result = exp(-pow(density * fc, 2.0));
    return 1.0 - clamp(result, 0.0, 1.0);
}
