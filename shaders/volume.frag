#version 330

in vec3 v_pos;
out vec4 fColor;

#include "uniforms.glsl"

uniform sampler3D u_volumeTexture;
uniform sampler1D u_transferFunction;
uniform vec3 u_volumeDimensions;
uniform vec3 u_gridVec1;
uniform vec3 u_gridVec2;
uniform vec3 u_gridVec3;

const int MAX_SAMPLES = 512;
const float STEP_SIZE = 1.0 / float(MAX_SAMPLES);

vec3 worldToTexture(vec3 worldPos) {
    // Create the transformation matrix from world space to texture space
    mat3 worldToTexMat = mat3(
        u_gridVec1.x, u_gridVec2.x, u_gridVec3.x,
        u_gridVec1.y, u_gridVec2.y, u_gridVec3.y,
        u_gridVec1.z, u_gridVec2.z, u_gridVec3.z
    );

    // Invert the matrix to go from world space to texture space
    mat3 texToWorldMat = inverse(worldToTexMat);

    // Transform the world position to texture coordinates
    vec3 texCoords = texToWorldMat * worldPos;

    // Scale the texture coordinates based on volume dimensions
    return texCoords / u_volumeDimensions;
}

vec4 sampleVolume(vec3 worldPos) {
    vec3 texCoords = worldToTexture(worldPos);

    // Check if the point is within the volume bounds
    if (any(lessThan(texCoords, vec3(0.0))) || any(greaterThan(texCoords, vec3(1.0)))) {
        return vec4(0.0);
    }

    float intensity = texture(u_volumeTexture, texCoords).r;
    return texture(u_transferFunction, intensity);
}

void main() {
/*
    vec3 rayDir = normalize(v_pos - u_cameraPosVec);
    vec3 rayStep = rayDir * STEP_SIZE;

    vec3 worldPos = v_pos;
    vec4 color = vec4(0.0);

    for (int i = 0; i < MAX_SAMPLES; i++) {
        vec4 sample = sampleVolume(worldPos);
        color.rgb += sample.rgb * sample.a * (1.0 - color.a);
        color.a += sample.a * (1.0 - color.a);

        if (color.a >= 0.95) {
            break;
        }

        worldPos += rayStep;
    }

    fColor = color;
	*/
	fColor = vec4(0.5, 0.5, 0.5, 0.5);
}
