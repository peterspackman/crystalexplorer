#version 330

in vec2 v_mapping;
in vec4 v_color;
flat in float v_r1squared;
flat in float v_max_angle; // in tan theta

out vec4 outputColor;
uniform float u_screenGamma;




void main()
{
    float r2 = 1.0;
    float dist2 = dot(v_mapping, v_mapping);
    if(dist2 > r2 || dist2 < v_r1squared) {
        discard;
    }
    if(v_max_angle != 0.0) {
        vec2 map_norm = normalize(v_mapping);
        if(map_norm.x < v_max_angle || (map_norm.y < 0)) discard;
    }
    vec3 colorGammaCorrected = pow(v_color.xyz, vec3(1.0/u_screenGamma));
    outputColor = vec4(colorGammaCorrected, v_color.a);
}
