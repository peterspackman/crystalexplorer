#version 330
layout(location = 0) in vec3 posA;
layout(location = 1) in vec3 posB;
layout(location = 2) in vec3 colorA;
layout(location = 3) in vec3 colorB;
layout(location = 4) in vec2 mapping;
layout(location = 5) in float lineWidth;

uniform mat4 u_projectionMat;
uniform mat4 u_viewMat;
uniform float u_scale;
uniform vec4 u_lightDirection;
uniform vec4 u_lightDiffuse;
uniform bool u_selection;
uniform mat4 u_modelViewMat;
uniform vec2 u_viewport_size;
uniform float u_lineScale;

out vec4 v_color;


const float boxCorrection = 1.5;

void main()
{
    float aspect = u_viewport_size.x / u_viewport_size.y;
    float linewidth = u_lineScale * lineWidth * u_scale;
    vec4 start = u_modelViewMat * vec4(posA, 1.0);
    vec4 end = u_modelViewMat * vec4(posB, 1.0);

    // clip space
    vec4 clipStart = u_projectionMat * start;
    vec4 clipEnd = u_projectionMat * end;

    // ndc space
    vec2 ndcStart = clipStart.xy / clipStart.w;
    vec2 ndcEnd = clipEnd.xy / clipEnd.w;

    // direction
    vec2 dir = ndcEnd - ndcStart;

    // account for clip-space aspect ratio
    dir.x *= aspect;
    dir = normalize( dir );

    // perpendicular to dir
    vec2 offset = vec2( dir.y, - dir.x );

    // undo aspect ratio adjustment
    dir.x /= aspect;
    offset.x /= aspect;

    // sign flip
    if ( mapping.x < 0.0 ) offset *= - 1.0;

    v_color = vec4(colorA, 1.0);
    // not used
    // // endcaps
    if ( mapping.y < 0.0 ) {
        offset += -dir;
    } else if ( mapping.y > 0.0 ) {
        offset += dir;
    }

    // adjust for linewidth
    offset *= linewidth;

    // adjust for clip-space to screen-space conversion
    offset /= u_viewport_size.y;

    // select end
    vec4 clip = ( mapping.y < 0.5 ) ? clipStart : clipEnd;

    // back to clip space
    offset *= clip.w;
    clip.xy += offset;
    gl_Position = clip;
}

