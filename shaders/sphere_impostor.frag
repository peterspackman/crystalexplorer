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
#include "common.glsl"



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

float intersectCorner(out vec3 cameraPos, out vec3 cameraNormal, out float alpha, vec2 mapping)
{
    vec3 cameraPlanePos = vec3(mapping * v_radius, 0.0) + v_cameraSpherePos;
    vec3 rayDirection = normalize(cameraPlanePos);

    // Use Inigo Quilez method for cleaner intersection
    vec2 intersection = sphIntersect(vec3(0.0), rayDirection, v_cameraSpherePos, v_radius);
    
    alpha = 1.0;
    if(intersection.x < 0.0) {
        // Calculate distance to sphere surface for antialiasing
        float dist_to_center = length(cameraPlanePos - v_cameraSpherePos);
        float edge_width = fwidth(dist_to_center) * 1.5;
        alpha = 1.0 - smoothstep(v_radius - edge_width, v_radius + edge_width, dist_to_center);
        
        if (alpha < 0.01) {
            return -1.0;
        }
        
        // For edge pixels, project onto sphere surface
        cameraPos = v_cameraSpherePos + normalize(cameraPlanePos - v_cameraSpherePos) * v_radius;
        cameraNormal = normalize(cameraPos - v_cameraSpherePos);
        return alpha;
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

float sphereIntersect(out vec3 cameraPos, out vec3 cameraNormal, out float alpha)
{
    cameraPos = vec3(0);
    cameraNormal = vec3(0);
    alpha = 1.0;
#ifdef USE_MULTISAMPLE

    vec3 pos, norm;
    float cornerAlpha;
    int numInside = 0;
    float totalAlpha = 0.0;

    vec2 dx = dFdx(v_mapping);
    vec2 dy = dFdy(v_mapping);
    if(intersectCorner(pos, norm, cornerAlpha, v_mapping - -dx - dy) >= 0) {
        cameraPos += pos;
        cameraNormal += norm;
        totalAlpha += cornerAlpha;
        numInside++;
    }
    if(intersectCorner(pos, norm, cornerAlpha, v_mapping - dx + dy) >= 0) {
        cameraPos += pos;
        cameraNormal += norm;
        totalAlpha += cornerAlpha;
        numInside++;
    }
    if(intersectCorner(pos, norm, cornerAlpha, v_mapping + dx - dy) >= 0) {
        cameraPos += pos;
        cameraNormal += norm;
        totalAlpha += cornerAlpha;
        numInside++;
    }
    if(intersectCorner(pos, norm, cornerAlpha, v_mapping + dx + dy) >= 0) {
        cameraPos += pos;
        cameraNormal += norm;
        totalAlpha += cornerAlpha;
        numInside++;
    }
    if(numInside <= 0) discard;
    cameraPos /= numInside;
    cameraNormal /= numInside;
    alpha = totalAlpha / numInside;
    return numInside / 4.0;
#else
    float result = intersectCorner(cameraPos, cameraNormal, alpha, v_mapping);
    if(result < 0.0) discard;
    return result;
#endif

}

void main()
{
    vec3 cameraPos;
    vec3 cameraNormal;
    float alpha;

    float coverage = sphereIntersect(cameraPos, cameraNormal, alpha);

    gl_FragDepth = calcDepth(cameraPos);
    
    // Transform to world space for lighting calculations
    vec4 worldSpacePos = u_modelViewMatInv * vec4(cameraPos, 1.0f);
    vec4 worldSpaceNormal = transpose(u_viewMat) * vec4(cameraNormal, 1.0f);
    
    // Use unified shading function
    outputColor = calculateShading(u_renderMode, v_color.xyz, u_cameraPosVec, worldSpacePos.xyz, worldSpaceNormal.xyz, alpha, v_object_id);
    outputColor = applyFog(outputColor, u_depthFogColor, u_depthFogOffset, u_depthFogDensity, gl_FragDepth);
}
