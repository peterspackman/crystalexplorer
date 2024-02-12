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
uniform vec4 u_lightSpecular;
uniform vec4 u_lightGlobalAmbient;
uniform vec2 u_attenuationClamp;
uniform vec4 u_selectionColor;
uniform bool u_selectionMode;
uniform float u_screenGamma;
uniform float u_materialRoughness;
uniform float u_materialMetallic;
uniform int u_numLights;

#define SELECTION_OUTLINE 1



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
        vec4 worldSpacePos = u_modelViewMatInv * vec4(cameraPos, 1.0f);
        vec4 worldSpaceNormal = transpose(u_viewMat) * vec4(cameraNormal, 1.0f);

        vec3 V = normalize(u_cameraPosVec - worldSpacePos.xyz);
        vec3 N = normalize(worldSpaceNormal.xyz);
        float c = dot(N, V);
        vec4 color = vec4(v_color);
     #ifdef SELECTION_OUTLINE
         if(v_selected == 1) {
             // could use c to mix as well
             color.xyz = mix(vec3(u_selectionColor), color.xyz, smoothstep(0.0, 0.5, c*c));
         }
         else {
             color.xyz = mix(vec3(0), color.xyz, smoothstep(0.0, 0.3, c*c));
         }
     #endif
        vec3 colorLinear = color.xyz;
        outputColor = vec4(pow(colorLinear, vec3(1.0 / u_screenGamma)), coverage * v_color.w*v_color.w);
//        vec4 colorLinear = Lighting(cameraPos, cameraNormal);
//        vec3 colorGammaCorrected = pow(colorLinear.xyz, vec3(1.0/u_screenGamma));
//        outputColor = vec4(colorGammaCorrected, 1.0f);
    }
}
