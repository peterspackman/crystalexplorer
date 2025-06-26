#version 330
#include "uniforms.glsl"
in vec4 v_colorA;
in vec4 v_colorB;
in vec3 v_normal;
in vec3 v_position;
in vec3 v_cylinderPosition;
flat in float v_length;
flat in float v_radius;
flat in vec4 v_selection_idA;
flat in vec4 v_selection_idB;
flat in int v_selectedA;
flat in int v_selectedB;
flat in int v_pattern;
out vec4 f_color;
int v_selected;
#define SELECTION_OUTLINE 1
#include "common.glsl"


vec2 generateUV(vec3 vertex) {
    const float PI = 3.14159265359;
    const float TWOPI = 2.0 * PI;


    float u = (atan(vertex.y, vertex.x) + PI) / TWOPI;
    float v = (vertex.z + 0.5); // Assuming z is in [-0.5, 0.5] range

    // Adjust U to reduce seam visibility
    u = fract(u + 0.5) - 0.5;
    return vec2(u, v);
}

vec3 applyPattern(vec3 baseColor, vec2 texCoord) {
    if(v_pattern == 0) return baseColor;
    float circumference = 2.0 * 3.14159 * v_radius;
    float cellSize = 0.08;
    float cellsAround = circumference / cellSize;
    float cellsAlong = v_length / cellSize;
    vec2 scaledTexCoord = vec2(texCoord.x * cellsAround, texCoord.y * cellsAlong);
    vec2 f = fract(scaledTexCoord);

    float thickness = 0.12; // Thickness of the bars
    float l = 0.8;    // Length of each bar of the '+'

    float horizontalBar = step(0.5 - l * 0.5, f.x) - step(0.5 + l * 0.5, f.x);
    horizontalBar *= smoothstep(0.5 - thickness, 0.5 - thickness + 0.01, f.y) - 
                     smoothstep(0.5 + thickness - 0.01, 0.5 + thickness, f.y);

    float verticalBar = step(0.5 - l * 0.5, f.y) - step(0.5 + l * 0.5, f.y);
    verticalBar *= smoothstep(0.5 - thickness, 0.5 - thickness + 0.01, f.x) - 
                   smoothstep(0.5 + thickness - 0.01, 0.5 + thickness, f.x);

    float cross = max(horizontalBar, verticalBar);

    vec2 aa = fwidth(scaledTexCoord);
    cross *= smoothstep(1.0, 0.0, length(aa) * 0.5);

    float brightness = perceivedBrightness(baseColor);

    vec3 contrastColor = brightness > 0.5 ? baseColor * 0.5 : baseColor * 1.5;

    return mix(baseColor, contrastColor, cross);
}

void main()
{
    // Determine which end of cylinder we're on and get appropriate color/selection
    vec4 color;
    vec3 selectionId;
    if(v_cylinderPosition.z < 0) {
        color = v_colorA;
        v_selected = v_selectedA;
        selectionId = v_selection_idA.xyz;
    }
    else {
        color = v_colorB;
        v_selected = v_selectedB;
        selectionId = v_selection_idB.xyz;
    }
    
    // Apply pattern to base color
    vec3 materialColor = applyPattern(color.xyz, generateUV(v_cylinderPosition));
    
    // Use unified shading
    f_color = calculateShading(u_renderMode, materialColor, u_cameraPosVec, v_position, v_normal, color.w, selectionId);
    f_color = applyFog(f_color, u_depthFogColor, u_depthFogOffset, u_depthFogDensity, gl_FragCoord.z);
}
