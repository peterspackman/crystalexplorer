// Combined flat and PBR shading functions

// Flat shading functions
vec3 flatWithNormalOutline(vec3 cameraPos, vec3 position, vec3 normal, vec3 color) {

    vec3 V = normalize(cameraPos - position);
    vec3 N = normalize(normal);
    float c = dot(N, V);
 #ifdef SELECTION_OUTLINE
     if(v_selected == 1) {
         // could use c to mix as well
         color = mix(vec3(u_selectionColor), color, smoothstep(0.0, 0.5, c*c));
     }
     else {
         color = mix(vec3(0), color, smoothstep(-0.5, 0.5, c));
     }
 #endif
     return color;
}

// PBR structures and functions
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

float perceivedBrightness(vec3 color) {
    return (0.299 * color.r + 0.587 * color.g + 0.114 * color.b);
}

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

vec3 GoochLighting(in vec3 cameraPosition, in vec3 position, in vec3 normal, in Lights lights, in vec3 materialColor) {
    vec3 N = normalize(normal);
    vec3 L = normalize(lights.positions[0].xyz - position);
    vec3 V = normalize(cameraPosition - position);
    
    // Adjusted Gooch parameters - less aggressive tinting
    float a = 0.2;
    float b = 0.6;
    float shininess = 60.0;
    
    // Diffuse component
    float NL = dot(N, L);
    float it = (1.0 + NL) / 2.0;
    
    // Use light color for warm, black for cool
    vec3 lightColor = lights.specular[0].rgb * 0.02;  // Tone down light intensity
    vec3 coolColor = vec3(0.0) + a * materialColor;  // Black + material
    vec3 warmColor = lightColor + b * materialColor;  // Light color + material
    
    // Mix between cool and warm based on lighting
    vec3 goochColor = (1.0 - it) * coolColor + it * warmColor;
    
    // Strong ambient contribution to preserve material colors
    vec3 ambient = lights.ambient * materialColor;
    vec3 color = mix(goochColor, materialColor, 0.6) + ambient * 0.8;
    
    // Specular highlights
    vec3 R = reflect(-L, N);
    float ER = clamp(dot(V, R), 0.0, 1.0);
    vec3 spec = vec3(1.0) * pow(ER, shininess) * 0.3; // Reduced spec intensity
    
    color += spec;
    
#ifdef SELECTION_OUTLINE
    if(v_selected == 1) {
        float c = dot(N, V);
        color = mix(vec3(u_selectionColor), color, smoothstep(0.0, 0.5, c*c));
    }
#endif

    return color;
}

float fogFactor(float density, float fc) {
    float result = exp(-pow(density * fc, 2.0));
    return 1.0 - clamp(result, 0.0, 1.0);
}

vec4 applyFog(vec4 color, vec3 fogColor, float offset, float density, float depth) {
    if(fogColor.r < 0.0) return color;
    // since we have inverted depth it's offset - depth not vice-versa
    return mix(color, vec4(fogColor, 1.0), fogFactor(density, clamp(offset - depth, 0, 1.0f)));
}

// Unified shading function that handles all render modes
vec4 calculateShading(int renderMode, vec3 materialColor, vec3 cameraPosition, vec3 worldPosition, vec3 worldNormal, float alpha, vec3 selectionId) {
    vec4 outputColor;
    
    if(renderMode == 0) {
        // Selection mode
        outputColor = vec4(selectionId, alpha);
    } 
    else if(renderMode == 1) {
        // Flat shading mode
        vec3 colorLinear = linearizeColor(materialColor, u_screenGamma);
        colorLinear = flatWithNormalOutline(cameraPosition, worldPosition, worldNormal, colorLinear);
        outputColor = vec4(unlinearizeColor(colorLinear, u_screenGamma), alpha);
    }
    else if(renderMode == 2) {
        // PBR shading mode
        PBRMaterial material;
        material.color = linearizeColor(materialColor, u_screenGamma);
        material.metallic = u_materialMetallic;
        material.roughness = u_materialRoughness;
        
        Lights lights;
        lights.positions = u_lightPos;
        lights.ambient = u_lightGlobalAmbient.xyz;
        lights.specular = u_lightSpecular;
        
        vec3 colorLinear = PBRLighting(cameraPosition, worldPosition, worldNormal, lights, material);
        outputColor = vec4(unlinearizeColor(colorLinear, u_screenGamma), alpha);
    }
    else if(renderMode == 3) {
        // Gooch shading mode
        vec3 colorLinear = linearizeColor(materialColor, u_screenGamma);
        
        Lights lights;
        lights.positions = u_lightPos;
        lights.ambient = u_lightGlobalAmbient.xyz;
        lights.specular = u_lightSpecular;
        
        colorLinear = GoochLighting(cameraPosition, worldPosition, worldNormal, lights, colorLinear);
        outputColor = vec4(unlinearizeColor(colorLinear, u_screenGamma), alpha);
    }
    else {
        // Default fallback to flat
        vec3 colorLinear = linearizeColor(materialColor, u_screenGamma);
        colorLinear = flatWithNormalOutline(cameraPosition, worldPosition, worldNormal, colorLinear);
        outputColor = vec4(unlinearizeColor(colorLinear, u_screenGamma), alpha);
    }
    
    return outputColor;
}
