#version 330
#include "uniforms.glsl"

in vec2 v_mapping;
in vec4 v_color;
in vec3 v_cameraSpherePos;
in vec3 v_spherePos;
in float v_radius;
flat in int v_selected;
flat in vec3 v_object_id;
out vec4 outputColor;

#define SELECTION_OUTLINE 1
#include "flat.glsl"
#include "pbr.glsl"



float calcDepth( in vec3 cameraPos ){
    vec2 clipZW = cameraPos.z * u_projectionMat[2].zw + u_projectionMat[3].zw;
    return 0.5 + 0.5 * clipZW.x / clipZW.y;
}


// Inigo Quilez sphere intersection - more robust
vec2 sphIntersect( in vec3 ro, in vec3 rd, in vec3 ce, float ra ) {
    vec3 oc = ro - ce;
    float b = dot( oc, rd );
    vec3 qc = oc - b*rd;
    float h = ra*ra - dot( qc, qc );
    if( h<0.0 ) return vec2(-1.0);
    h = sqrt( h );
    return vec2( -b-h, -b+h );
}

float intersectCorner(out vec3 cameraPos, out vec3 cameraNormal, vec2 mapping)
{
    vec3 cameraPlanePos = vec3(mapping * v_radius, 0.0) + v_cameraSpherePos;
    vec3 rayDirection = normalize(cameraPlanePos);

    // Use Inigo Quilez method for cleaner intersection
    vec2 intersection = sphIntersect(vec3(0.0), rayDirection, v_cameraSpherePos, v_radius);
    
    if(intersection.x < 0.0) {
        return -1.0;
    }

    // Pick the first intersection
    float intersectionDepth = intersection.x;
    if (intersection.y > 0.0 && intersection.y < intersection.x) {
        intersectionDepth = intersection.y;
    }

    cameraPos = rayDirection * intersectionDepth;
    cameraNormal = normalize(cameraPos - v_cameraSpherePos);
    return 1.0;
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
    if(u_renderMode == 0) {
        outputColor = vec4(v_object_id, 1.0f);
    }
    else if(u_renderMode == 1) {
        // Flat shading mode
        vec4 color = v_color;
        vec3 colorLinear = linearizeColor(color.xyz, u_screenGamma);
        vec4 worldSpacePos = u_modelViewMatInv * vec4(cameraPos, 1.0f);
        vec4 worldSpaceNormal = transpose(u_viewMat) * vec4(cameraNormal, 1.0f);
        colorLinear = flatWithNormalOutline(u_cameraPosVec, worldSpacePos.xyz, worldSpaceNormal.xyz, colorLinear);
        outputColor = vec4(unlinearizeColor(colorLinear, u_screenGamma), color.w);
        outputColor = applyFog(outputColor, u_depthFogColor, u_depthFogOffset, u_depthFogDensity, gl_FragDepth);
    }
    else {
        // PBR shading mode
        PBRMaterial material;
        material.color = linearizeColor(v_color.xyz, u_screenGamma);
        material.metallic = u_materialMetallic;
        material.roughness = u_materialRoughness;
        Lights lights;
        lights.positions = u_lightPos;
        lights.ambient = u_lightGlobalAmbient.xyz;
        lights.specular = u_lightSpecular;
        vec4 worldSpacePos = u_modelViewMatInv * vec4(cameraPos, 1.0f);
        vec4 worldSpaceNormal = transpose(u_viewMat) * vec4(cameraNormal, 1.0f);

        // Use world space lighting like ellipsoid renderer
        vec3 colorLinear = PBRLighting(u_cameraPosVec, worldSpacePos.xyz, worldSpaceNormal.xyz, lights, material);
        outputColor = vec4(unlinearizeColor(colorLinear, u_screenGamma), v_color.w);
        outputColor = applyFog(outputColor, u_depthFogColor, u_depthFogOffset, u_depthFogDensity, gl_FragDepth);
    }
}
