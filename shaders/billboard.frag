#version 330

in vec2 v_texcoord;
out vec4 fragColor;

uniform sampler2D u_texture;
uniform vec3 u_textColor;
uniform vec3 u_textOutlineColor;
uniform float u_textSDFSmoothing;
uniform float u_textSDFBuffer;
uniform float u_textSDFOutline;

void main()
{
    vec2 clampedTexCoord = clamp(v_texcoord, 0.0, 1.0);

    float distance = texture(u_texture, v_texcoord).r;
    distance -= u_textSDFBuffer;
    float smoothing = u_textSDFSmoothing * fwidth(distance);
    float textAlpha = smoothstep(0.5 + smoothing, 0.5 - smoothing, distance);
    float outlineAlpha = smoothstep(0.5 + u_textSDFOutline + smoothing,
                                    0.5 + u_textSDFOutline - smoothing,
                                    distance);
    vec3 color = mix(u_textOutlineColor, u_textColor, textAlpha);
    float alpha = max(textAlpha, outlineAlpha);

    // Additional alpha fade near texture edges
    vec2 edgeDistance = min(clampedTexCoord, 1.0 - clampedTexCoord);
    float edgeFade = smoothstep(0.0, 0.02, min(edgeDistance.x, edgeDistance.y));
    alpha *= edgeFade;

    fragColor = vec4(color, alpha);

    // Discard nearly transparent fragments
    if (alpha < 0.01) {
        discard;
    }

}
