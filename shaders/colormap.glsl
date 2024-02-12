uniform mat3 u_colorscheme;
uniform float u_minColorValue;
uniform float u_maxColorValue;

vec4 colormap(float value, float alpha) {
    int index = 0;
    float factor = 0;
    if (value < 0) {
        factor = 1.0 - value / u_minColorValue;
        index = 2;
    }
    else factor = 1.0 - value / u_maxColorValue;

    vec3 color = u_colorscheme[index];
    vec3 mid = u_colorscheme[1];
    vec3 result = color + (mid - color) * factor;
    return vec4(result, alpha);
}
