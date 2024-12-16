#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D depthTexture;
uniform samplerBuffer sphereData;
uniform int numSpheres;
uniform mat4 u_modelViewProjectionMat;
uniform float u_scale;

const float DEPTH_EPSILON = 0.001; // Adjust this value as needed

void main() {
    float fragDepth = texture(depthTexture, TexCoords).r;

    if(fragDepth == 0.0) {
        FragColor = vec4(1.0, 1.0, 1.0, 0.0);
        return;
    }

    vec3 fragPos = vec3(TexCoords * 2.0 - 1.0, 2.0 * fragDepth - 1.0);

    float totalOcclusion = 0.0;
    float totalWeight = 0.0;

    for(int i = 0; i < numSpheres; i++) {
        vec4 sphere = texelFetch(sphereData, i);
        vec4 sphereClip = u_modelViewProjectionMat * vec4(sphere.xyz, 1.0);
        sphereClip.xyz /= sphereClip.w;

        // Skip if we're at this sphere's depth
        float depthDiff = abs(sphereClip.z - fragPos.z);
        if(depthDiff < DEPTH_EPSILON) {
            continue;
        }

        float radius = sphere.w / (u_scale * sphereClip.w);
        vec3 toSphere = sphereClip.xyz - fragPos;
        float dist = length(toSphere.xy); // Only consider xy distance for occlusion

        // Only consider spheres in front of current fragment
        if(sphereClip.z > fragPos.z) {
            float influenceRadius = radius * 2.0;
            if(dist < influenceRadius) {
                float occlusion = 1.0 - smoothstep(0.0, influenceRadius, dist);
                // Weight by z-difference for more realistic falloff
                float zWeight = 1.0 - smoothstep(0.0, radius * 3.0, depthDiff);
                occlusion *= zWeight;

                totalOcclusion += occlusion;
                totalWeight += 1.0;
            }
        }
    }

    float ao = 1.0;
    if(totalWeight > 0.0) {
        ao = 1.0 - (totalOcclusion / totalWeight);
        ao = mix(1.0, ao, 0.7); // Adjust strength
    }

    FragColor = vec4(ao, ao, ao, 1.0);
}
