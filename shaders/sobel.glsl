#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform vec2 resolution;

const mat3 gx = mat3(-1, 0, 1,
                    -2, 0, 2,
                    -1, 0, 1);

const mat3 gy = mat3(-1, -2, -1,
                    0,  0,  0,
                    1,  2,  1);

void main()
{
    vec2 pixel = 1.0 / resolution; // size of one pixel

    mat3 I;
    for(int y = -1; y <= 1; y++)
    {
        for(int x = -1; x <= 1; x++)
        {
            vec3 color = texture(screenTexture, TexCoords + vec2(x, y) * pixel).rgb;
            I[y + 1][x + 1] = color.r; // Assuming grayscale, adjust if working with color images
        }
    }

    float gradX = dot(gx[0], I[0]) + dot(gx[1], I[1]) + dot(gx[2], I[2]);
    float gradY = dot(gy[0], I[0]) + dot(gy[1], I[1]) + dot(gy[2], I[2]);
    float grad = length(vec2(gradX, gradY)); // Magnitude of the gradient

    FragColor = vec4(vec3(grad), 1.0);
}
