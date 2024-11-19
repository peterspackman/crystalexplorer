#version 330

in vec4 v_color;
flat in vec4 v_selectionColor;

#include "uniforms.glsl"

out vec4 fColor;

void main() {
    if(u_renderMode == 0) {
        fColor = v_selectionColor;
    }
    else {
        vec3 colorGammaCorrected = pow(v_color.xyz, vec3(1.0/u_screenGamma));
        fColor = vec4(colorGammaCorrected, v_color.a);
    }
}
