#version 330
#define NEAR_CLIP
in vec3 axis; // Cylinder axis
in vec4 base_radius; // base position and cylinder radius packed into a vec4
in vec4 end_b; // End position and "b" flag which indicates whether pos1/2 is flipped
in vec3 U; // axis, U, V form orthogonal basis aligned to the cylinder
in vec3 V;
in vec4 w; // The position of the vertex after applying the mapping

in vec3 v_colorA;
in vec3 v_colorB;
in vec3 v_light;
in vec3 v_selection_id;
flat in int v_selectedA;
flat in int v_selectedB;
out vec4 outputColor;

bool interior = false;
uniform mat4 u_projectionMat;
uniform float u_ortho;
uniform float u_screenGamma;
const float clipNear = 0.01;

uniform int u_numLights;
uniform mat4 u_lightPos;
uniform vec2 u_attenuationClamp;
uniform mat4 u_lightSpecular;
uniform vec4 u_lightGlobalAmbient;
uniform bool u_selectionMode;
uniform vec4 u_selectionColor;
uniform float u_materialRoughness;
uniform float u_materialMetallic;

int v_selected = 0;

float distSq3( vec3 v3a, vec3 v3b ){
    return (
        ( v3a.x - v3b.x ) * ( v3a.x - v3b.x ) +
        ( v3a.y - v3b.y ) * ( v3a.y - v3b.y ) +
        ( v3a.z - v3b.z ) * ( v3a.z - v3b.z )
    );
}

// Calculate depth based on the given camera position.
float calcDepth( in vec3 cameraPos ){
    vec2 clipZW = cameraPos.z * u_projectionMat[2].zw + u_projectionMat[3].zw;
    return 0.5 + 0.5 * clipZW.x / clipZW.y;
}

float calcClip( vec3 cameraPos ){
    return dot( vec4( cameraPos, 1.0 ), vec4( 0.0, 0.0, 1.0, clipNear - 0.5 ) );
}

#define SELECTION_OUTLINE 1
/*
  COPIED FROM pbr.glsl, don't modify directly
  */

struct PBRMaterial {
    vec3 color;
    float metallic;
    float roughness;

};

struct Lights {
    mat4 positions;
    vec3 ambient;
    mat4 specular;
};

const float M_PI = 3.14159265359;

float DistributionGGX(float NdotH, float roughness)
{
    float a2 = roughness * roughness;
    float f = (NdotH * a2 - NdotH) * NdotH + 1.0;
    return a2 / (M_PI * f * f);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}
float GeometrySmith(float NdotL, float NdotV, float roughness)
{
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}


vec3 PBRLighting(in vec3 cameraPosition, in vec3 position, in vec3 normal, in Lights lights, in PBRMaterial material) {
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, material.color, material.metallic);
    vec3 Lo = vec3(0.0);

    vec3 V = normalize(cameraPosition - position);
    vec3 N = normalize(normal);
    float ao = 1.0;

    // for when we have more than one light
    for(int i = 0; i < u_numLights; i++) {
        vec3 L = normalize(lights.positions[i].xyz - position);
        float distance = length(lights.positions[i].xyz - position);
        float attenuation = 2.0;
        vec3 radiance = lights.specular[i].rgb * attenuation;
        vec3 H = normalize(V + L);
        float NdotV = abs(dot(N, V)) + 1e-5;
        float NdotL = clamp(dot(N, L), 0.0, 1.0);
        float NdotH = clamp(dot(N, H), 0.0, 1.0);
        float LdotH = clamp(dot(L, H), 0.0, 1.0);
        // Cook-Torrance BRDF
        float D = DistributionGGX(NdotH, material.roughness);
        float G = GeometrySmith(NdotL, NdotV, material.roughness);
        vec3 F = FresnelSchlick(LdotH, F0);

        vec3 numerator    = D * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.001) * max(dot(N, L), 0.0) + 0.00000000001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;

        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - material.metallic;

        // scale light by NdotL
        // add to outgoing radiance Lo
        Lo += (kD * material.color / M_PI + specular) * radiance * NdotL;
    }
    vec3 ambient = lights.ambient * material.color * ao;
    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));
#ifdef SELECTION_OUTLINE
    if(v_selected == 1) {
        float c = dot(N, V);
        // could use c to mix as well
        color = mix(vec3(u_selectionColor), color, smoothstep(0.0, 0.5, c*c));
    }
#endif

    return color;
}


/*
  END pbr.glsl section
  */



void main(){

    // The coordinates of the fragment, somewhere on the aligned mapped box
    vec3 ray_target = w.xyz / w.w;

    // unpack variables
    vec3 base = base_radius.xyz; // center of the base (far end), in modelView space
    float vRadius = base_radius.w; // radius in model view space
    vec3 end = end_b.xyz; // center of the end (near end) in modelView
    float b = end_b.w; // b is flag to decide if we're flipping this cylinder (see vertex shader)

    vec3 ray_origin = vec3(0.0); // Camera position for perspective mode
    vec3 ortho_ray_direction =  vec3(0.0, 0.0, 1.0); // Ray is cylinder -> camera
    vec3 persp_ray_direction = normalize(ray_origin - ray_target); // Ditto

    vec3 ray_direction = mix(persp_ray_direction, ortho_ray_direction, u_ortho);

    // basis is the rotation matrix for cylinder-aligned coords -> modelView
    // (or post-multiply to reverse, see below)
    mat3 basis = mat3( U, V, axis );

    // diff is vector from center of cylinder to target
    vec3 diff = ray_target - 0.5 * (base + end);

    // P is point transformed back to cylinder-aligned (post-multiplied)
    vec3 P = diff * basis;

    // angle (cos) between cylinder cylinder_axis and ray direction
    // axis looks towards camera (see vertex shader)
    float dz = dot( axis, ray_direction );

    float radius2 = vRadius*vRadius;

    // calculate distance to the cylinder from ray origin
    vec3 D = vec3(dot(U, ray_direction),
                dot(V, ray_direction),
                dz);
    float a0 = P.x*P.x + P.y*P.y - radius2;
    float a1 = P.x*D.x + P.y*D.y;
    float a2 = D.x*D.x + D.y*D.y;

    // calculate a dicriminant of the above quadratic equation
    float d = a1*a1 - a0*a2;
    if (d < 0.0) {
        // Point outside of the cylinder, becomes significant in perspective mode when camera is close
        // to the cylinder
        discard;
    }
    float dist = (-a1 + sqrt(d)) / a2;

    // point of intersection on cylinder surface (how far 'behind' the box surface the curved section of the cylinder would be)
    vec3 surface_point = ray_target + dist * ray_direction;

    vec3 base_to_surface = surface_point - base;
    // Calculates surface normal (of cylinder side) by finding point along cylinder axis in line with tmp_point
    vec3 _normal = normalize( base_to_surface - axis * dot(base_to_surface, axis) );

    // test caps
    float base_cap_test = dot( base_to_surface, axis );
    float end_cap_test = dot((surface_point - end), axis);

    // to calculate caps, simply check the angle between
    // the point of intersection - cylinder end vector
    // and a cap plane normal (which is the cylinder cylinder_axis)
    // if the angle < 0, the point is outside of cylinder
    // test base cap

    #ifndef CAP
        vec3 new_point2 = ray_target + ( (-a1 - sqrt(d)) / a2 ) * ray_direction;
        vec3 tmp_point2 = new_point2 - base;
    #endif

    // flat
    if (base_cap_test < 0.0) // The (extended) surface point falls outside the cylinder - beyond the base (away from camera)
    {
        // ray-plane intersection
        // Ortho mode - surface point is ray_target
        float dNV;
        float near;
        vec3 front_point;
        if ( u_ortho == 1.0 ) {
            front_point = ray_target;
        } else {
            dNV = dot(-axis, ray_direction);
            // @fredludlow: Explicit discard is not required here?
            // if (dNV < 0.0) {
            //     discard;
            // }
            near = dot(-axis, (base)) / dNV;
            front_point = ray_direction * near + ray_origin;
        }
        // within the cap radius?
        if (dot(front_point - base, front_point-base) > radius2) {
            discard;
        }

        #ifdef CAP
            surface_point = front_point;
            _normal = axis;
        #else
            // Calculate interior point
            surface_point = ray_target + ( (-a1 - sqrt(d)) / a2 ) * ray_direction;
            dNV = dot(-axis, ray_direction);
            near = dot(axis, end) / dNV;
            new_point2 = ray_direction * near + ray_origin;
            if (dot(new_point2 - end, new_point2-base) < radius2) {
                discard;
            }
            interior = true;
        #endif
    }

    // test end cap


    // flat
    if( end_cap_test > 0.0 )
    {
        // @fredludlow: NOTE: Perspective and ortho behaviour is quite different here. In perspective mode
        // it is possible to see the inside face of the mapped aligned box and these points should be
        // discarded. This occcurs when the camera is focused on one end of the cylinder and the cylinder
        // is not quite in line with the camera (In orthographic mode this view is not possible).
        // It is also possible to see the back face of the near (end) cap when looking nearly side-on.
        float dNV;
        float near;
        vec3 end_point;
        if ( u_ortho == 1.0 ) {
            end_point = ray_target;
        } else {
            dNV = dot(axis, ray_direction);
            if (dNV < 0.0) {
                // Viewing inside/back face of end-cap
                discard;
            }
            near = dot(axis, end) / dNV;
            end_point = ray_direction * near + ray_origin;
        }

        // within the cap radius?
        if( dot(end_point - end, end_point-base) > radius2 ) {
            discard;

        }
        #ifdef CAP
            surface_point = end_point;
            _normal = axis;
        #else
            // Looking down the tube at an interior point, but check to see if interior point is
            // within range:
            surface_point = ray_target + ( (-a1 - sqrt(d)) / a2 ) * ray_direction;
            dNV = dot(-axis, ray_direction);
            near = dot(-axis, (base)) / dNV;
            new_point2 = ray_direction * near + ray_origin;
            if (dot(new_point2 - base, new_point2-base) < radius2) {
                // Looking down the tube, which should be open-ended
                discard;
            }
            interior = true;
        #endif
    }

    gl_FragDepth = calcDepth( surface_point );


    #ifdef NEAR_CLIP
        if( calcClip( surface_point ) > 0.0 ){
            dist = (-a1 - sqrt(d)) / a2;
            surface_point = ray_target + dist * ray_direction;
            if( calcClip( surface_point ) > 0.0 ) {
                discard;
            }
            interior = true;
            gl_FragDepth = calcDepth( surface_point );
            if( gl_FragDepth >= 0.0 ){
                gl_FragDepth = max( 0.0, calcDepth( vec3( - ( clipNear - 0.5 ) ) ) + ( 0.0000001 / vRadius ) );
            }
        }else if( gl_FragDepth <= 0.0 ){
            dist = (-a1 - sqrt(d)) / a2;
            surface_point = ray_target + dist * ray_direction;
            interior = true;
            gl_FragDepth = calcDepth( surface_point );
            if( gl_FragDepth >= 0.0 ){
                gl_FragDepth = 0.0 + ( 0.0000001 / vRadius );
            }
        }
    #else
        if( gl_FragDepth <= 0.0 ){
            dist = (-a1 - sqrt(d)) / a2;
            surface_point = ray_target + dist * ray_direction;
            interior = true;
            gl_FragDepth = calcDepth( surface_point );
            if( gl_FragDepth >= 0.0 ){
                gl_FragDepth = 0.0 + ( 0.0000001 / vRadius );
            }
        }
    #endif

    // this is a workaround necessary for Mac
    // otherwise the modified fragment won't clip properly
    if (gl_FragDepth < 0.0) {
        discard;
    }
    if (gl_FragDepth > 1.0) {
        discard;
    }

    if(u_selectionMode) {
        outputColor = vec4(v_selection_id, 1.0);
    } else {
        vec3 vViewPosition = -surface_point;
        vec3 vNormal = _normal;
        vec3 vColor;
        bool selected = false;

        if( distSq3( surface_point, end ) < distSq3( surface_point, base ) ) {
            if( b < 0.0 ) {
                vColor = v_colorA;
                selected = v_selectedA > 0;
            }
            else {
                vColor = v_colorB;
                selected = v_selectedB > 0;

            }
        }
        else {
            if( b > 0.0 ) {
                vColor = v_colorA;
                selected = v_selectedA > 0;

            }
            else {
                vColor = v_colorB;
                selected = v_selectedB > 0;
            }
        }
        v_selected = selected ? 1 : 0;
        vec3 normal = normalize( vNormal );


        PBRMaterial material;
        material.color = vColor.xyz;
        material.metallic = u_materialMetallic;
        material.roughness = u_materialRoughness;
        Lights lights;
        lights.positions = u_lightPos;
        lights.ambient = u_lightGlobalAmbient.xyz;
        lights.specular = u_lightSpecular;

        // since we're passing things through in camera space, the camera is located at the origin
        vec3 colorLinear = PBRLighting(vec3(0), -vViewPosition, vNormal, lights, material);
        outputColor = vec4(pow(colorLinear, vec3(1.0 / u_screenGamma)), 1.0);
    }
}
