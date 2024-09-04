#version 330

// Input attributes
in highp vec4 v_color;
in highp vec3 v_normal;
in highp vec3 v_position;
in highp vec3 v_spherePosition;
flat in highp vec4 v_selection_id;
flat in int v_selected;
flat in int v_showLines;

// Output G-Buffer
layout (location = 0) out highp vec4 g_albedo;
layout (location = 1) out highp vec4 g_position;
layout (location = 2) out highp vec4 g_normal;
layout (location = 3) out highp vec4 g_material;

#include "uniforms.glsl"

vec3 applyLines(vec3 color) {
    if(v_showLines == 1) {
        vec3 apos = abs(v_spherePosition);
        vec3 lineColor = vec3(0.05, 0.05, 0.05);
        if(apos.x < u_ellipsoidLineWidth) color = mix(lineColor, color, smoothstep(0, u_ellipsoidLineWidth, apos.x));
        if(apos.y < u_ellipsoidLineWidth) color = mix(lineColor, color, smoothstep(0, u_ellipsoidLineWidth, apos.y));
        if(apos.z < u_ellipsoidLineWidth) color = mix(lineColor, color, smoothstep(0, u_ellipsoidLineWidth, apos.z));
    }
    return color;
}

vec3 linearizeColor(vec3 color, float gamma) {
    return pow(color, vec3(gamma));
}

void main()
{
    // Output position
    g_position = vec4(v_position, 1.0);
    // Output normal
    g_normal = vec4(normalize(v_normal), 1.0);

    // Output albedo and specular
    vec3 albedo = linearizeColor(v_color.rgb, u_screenGamma);
    g_albedo.rgb = applyLines(albedo);
    g_albedo.a = v_color.a;
    g_material.r = v_selection_id.r;
    g_material.g = gl_FragCoord.z;
    g_material.b = v_selected;
}
