#version 330
in vec4 v_color;

out vec4 outputColor;
uniform float u_screenGamma;




void main()
{
    vec3 colorGammaCorrected = pow(v_color.xyz, vec3(1.0/u_screenGamma));
    outputColor = vec4(colorGammaCorrected, v_color.a);
}
