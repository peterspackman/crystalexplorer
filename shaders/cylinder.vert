#version 330
#include "uniforms.glsl"
layout(location = 0) in vec3 vertex;
layout(location = 1) in float radius;
layout(location = 2) in vec3 a;
layout(location = 3) in vec3 b;
layout(location = 4) in vec3 colorA;
layout(location = 5) in vec3 colorB;
layout(location = 6) in vec3 selectionIdA;
layout(location = 7) in vec3 selectionIdB;

out vec4 v_colorA;
out vec4 v_colorB;
out vec3 v_normal;
out vec3 v_position;
out vec3 v_cylinderPosition;
flat out vec4 v_selection_idA;
flat out vec4 v_selection_idB;
flat out int v_selectedA;
flat out int v_selectedB;
flat out int v_pattern;

mat4 rotation_from_axis_angle(float angle, vec3 axis) {
    float ct = cos(angle);
    float c1 = 1 - ct;
    float st = sin(angle);
    float x = axis.x, y = axis.y, z = axis.z;
    float x2 = x * x, y2 = y * y, z2 = z*z;
    return mat4(
        ct + x2 * c1, x * y * c1 - z * st, x*z*c1 + y * st, 0,
        x*y*c1 + z*st, ct + y2 * c1, y*z*c1 - x*st,  0,
        x*z*c1 - y *st, y*z*c1 + x*st, ct + z2 * c1, 0,
        0, 0, 0, 1
    );
}


void main()
{
    vec3 ab = b - a;
    vec3 T = a + 0.5 * ab;
    float l = length(ab);
    vec3 u_ab = ab / l;

    float zcomp = ab.z / l;
    float angle = acos(zcomp);
    float n = length(vec2(ab));
    vec3 ax = vec3(0, 0, 0);

    if (n > 1e-5) { // This handles the case when 0 < theta < 180 etc.
        ax.x = -ab.y / n;
        ax.y = ab.x / n;
    }
    else { // This handles the case when n<0.0001 and theta = 180, 360, etc.,
             // but != 0.
        ax.x = 0.0;
        ax.y = 1.0;
    }

    v_pattern = (radius > 0.0) ? 0 : 1;
    float r = abs(radius);

    v_cylinderPosition = vertex;
    v_selectedA = (colorA.x < 0) ? 1 : 0;
    v_selectedB = (colorB.x < 0) ? 1 : 0;
    mat4 scale = mat4(r, 0, 0, 0,
                      0, r, 0, 0,
                      0, 0, l, 0,
                      0, 0, 0, 1);
    mat4 transform = transpose(rotation_from_axis_angle(angle, ax)) * scale;
    mat4 normalTransform = inverse(transpose(transform));
    vec4 posTransformed = transform * vec4(vertex, 1);
    v_normal = normalize(mat3(normalTransform) * vertex);
    v_position = posTransformed.xyz + T;
    v_colorA = vec4(abs(colorA), 1.0f);
    v_colorB = vec4(abs(colorB), 1.0f);

    v_selection_idA = vec4(selectionIdA, 1.0);
    v_selection_idB = vec4(selectionIdB, 1.0);

    gl_Position = u_modelViewProjectionMat * vec4(v_position, 1.0);
}

