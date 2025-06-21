#version 330

layout(location = 0) in vec3 pointA;
layout(location = 1) in vec3 pointB;
layout(location = 2) in vec3 colorA;
layout(location = 3) in vec3 colorB;
layout(location = 4) in vec3 mapping;
layout(location = 5) in vec3 selection_id;
layout(location = 6) in float radius;


out vec3 axis; // Cylinder axis
out vec4 base_radius; // base position and cylinder radius packed into a vec4
out vec4 end_b; // End position and "b" flag which indicates whether pos1/2 is flipped
out vec3 v_pointA; // Original pointA for color calculation
out vec3 v_pointB; // Original pointB for color calculation
out vec3 U; // axis, U, V form orthogonal basis aligned to the cylinder
out vec3 V;
out vec4 w; // The position of the vertex after applying the mapping

out vec3 v_colorA;
out vec3 v_colorB;
out float v_blend;
out vec3 v_selection_id;
out vec3 v_light;
flat out int v_selected;

uniform mat4 u_modelViewMat;
uniform mat4 u_modelViewMatInv;
uniform mat4 u_projectionMat;
uniform float u_ortho;
uniform float u_scale;
uniform vec3 u_cameraPosVec;
uniform mat3 u_normalMat;
uniform vec4 u_lightDiffuse;

void main(){

    v_selection_id = selection_id;

    // Pass original endpoints in view space for consistent color calculation
    v_pointA = (u_modelViewMat * vec4(pointA, 1.0)).xyz;
    v_pointB = (u_modelViewMat * vec4(pointB, 1.0)).xyz;

    base_radius.w = radius * u_scale * u_scale;

    vec3 center = 0.5 * (pointA + pointB);

    vec3 dir = normalize(pointB - pointA);
    float ext = length(pointB - pointA) / 2.0; // Half-length of cylinder
    // Determine which direction the camera is in (in molecule coords)
    // using cameraPosition fails on some machines, not sure why
    // vec3 cam_dir = normalize( cameraPosition - mix( center, vec3( 0.0 ), ortho ) );
    vec3 cam_dir;
    if( u_ortho == 0.0 ){
        cam_dir = ( u_modelViewMatInv * vec4(0, 0, 0, 1) ).xyz - center;
        // Equivalent to, but see note above
        // cam_dir = normalize( cameraPosition - center );
    }else{
        // Orthographic camera looks along -Z
        cam_dir = ( u_modelViewMatInv * vec4(0, 0, 1, 0) ).xyz;
    }
    cam_dir = normalize( cam_dir );
    // ldir is the cylinder's direction (center->end) in model coords
    // It will always point towards the camera
    vec3 ldir;

    float b = dot( cam_dir, dir );
    end_b.w = b;
    // direction vector looks away, so flip
    if( b < 0.0 )
       ldir = -ext * dir;
    // direction vector already looks in my direction
    else
       ldir = ext * dir;

    // left, up and ldir are orthogonal coordinates aligned with cylinder (ldir)
    // scaled to the length and radius of the box
    vec3 left = radius * normalize( cross( cam_dir, ldir ) );
    vec3 up = radius * normalize( cross( left, ldir ) );


    // Normalized versions of ldir, up and left, these can be used to convert
    // from modelView <-> cylinder-aligned
    axis = normalize(u_normalMat * ldir );
    U = normalize(u_normalMat * up );
    V = normalize(u_normalMat * left );

    // Transform the base (the distant cap) and pack its coordinate
    vec4 base4 = u_modelViewMat * vec4(center - ldir, 1.0);
    base_radius.xyz = base4.xyz / base4.w;
    // Similarly with the end (the near cap)
    vec4 end4 = u_modelViewMat * vec4( center + ldir, 1.0 );
    end_b.xyz = end4.xyz / end4.w;

    // w is effective coordinate (apply the mapping)
    w = u_modelViewMat * vec4(
        center + mapping.x*ldir + mapping.y*left + mapping.z*up, 1.0
    );

    gl_Position = u_projectionMat * w;
    
    // Pass both colors to fragment shader
    v_colorA = abs(colorA);
    v_colorB = abs(colorB);
    
    // Set blend factor based on original cylinder direction (pointB - pointA)
    // not the view-flipped ldir, so colors don't flip with camera
    vec3 vertex_pos = center + mapping.x*ldir + mapping.y*left + mapping.z*up;
    float axis_pos = dot(vertex_pos - pointA, normalize(pointB - pointA));
    float cylinder_len = length(pointB - pointA);
    v_blend = (axis_pos > cylinder_len * 0.5) ? 1.0 : 0.0;
    
    // Set selection based on either color being selected
    v_selected = (colorA.x < 0 || colorB.x < 0) ? 1 : 0;

    // avoid clipping (1.0 seems to induce flickering with some drivers)
    // Is this required?
//    gl_Position.z = 0.99;
}
