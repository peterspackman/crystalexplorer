#version 330
#include "uniforms.glsl"

in highp vec4 v_color;
in highp vec3 v_normal;
in highp vec3 v_position;
in highp vec3 v_spherePosition;
flat in highp vec4 v_selection_id;
flat in int v_selected;
flat in int v_showLines;
out highp vec4 f_color;

#define SELECTION_OUTLINE 1
#include "common.glsl"

vec3 applyLines(vec3 color) {
    if(v_showLines == 1) {
        vec3 apos = abs(v_spherePosition);
        float brightness = perceivedBrightness(color);
        vec3 lineColor = brightness > 0.1 ? vec3(0.01) : vec3(0.95);

        float adjustedLineWidth = 0.5 * u_ellipsoidLineWidth / u_scale;
        float featherWidth = adjustedLineWidth * 0.9; // Adjust this factor to control the smoothness
        
        if(apos.x < adjustedLineWidth) 
            color = mix(lineColor, color, smoothstep(adjustedLineWidth - featherWidth, adjustedLineWidth + featherWidth, apos.x));
        if(apos.y < adjustedLineWidth) 
            color = mix(lineColor, color, smoothstep(adjustedLineWidth - featherWidth, adjustedLineWidth + featherWidth, apos.y));
        if(apos.z < adjustedLineWidth) 
            color = mix(lineColor, color, smoothstep(adjustedLineWidth - featherWidth, adjustedLineWidth + featherWidth, apos.z));
    }
    return clamp(color, 0.0, 1.0);
}


void main()
{
    // Apply lines to base color
    vec3 materialColor = applyLines(v_color.xyz);
    
    // Use unified shading
    f_color = calculateShading(u_renderMode, materialColor, u_cameraPosVec, v_position, v_normal, v_color.w, v_selection_id.xyz);
    f_color = applyFog(f_color, u_depthFogColor, u_depthFogOffset, u_depthFogDensity, gl_FragCoord.z);
}
