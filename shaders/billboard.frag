#version 330

in vec2 v_mapping;
in vec3 v_cameraPos;
out vec4 outputColor;

uniform float u_screenGamma;
uniform mat4 u_projectionMat;
uniform sampler2D u_texture;
uniform float u_textSDFBuffer;
uniform float u_textSDFSmoothing;
uniform float u_textSDFOutline;
uniform vec3 u_textColor;
uniform vec3 u_textOutlineColor;

vec3 gammaCorrection(vec3 color, float gamma) {
    return pow(color, vec3(1.0/gamma));
}

void main()
{
    highp vec2 tx = clamp(v_mapping, 0, 1);
    float smoothing = u_textSDFSmoothing;
    float buf = u_textSDFBuffer;
    float distance = texture(u_texture, tx).r;
    float border = smoothstep(buf + u_textSDFOutline - smoothing, buf + u_textSDFOutline + smoothing, distance);
    float alpha = smoothstep(buf - smoothing, buf + smoothing, distance);
    outputColor = vec4(mix(u_textOutlineColor, u_textColor, border), 1.) * alpha;
}
