#version 330
#include "uniforms.glsl"

in vec3 axis; // Cylinder axis
in vec4 base_radius; // base position and cylinder radius packed into a vec4
in vec4 end_b; // End position and "b" flag which indicates whether pos1/2 is flipped
in vec3 v_pointA; // Original pointA for color calculation
in vec3 v_pointB; // Original pointB for color calculation
in vec3 U; // axis, U, V form orthogonal basis aligned to the cylinder
in vec3 V;
in vec4 w; // The position of the vertex after applying the mapping

in vec3 v_colorA;
in vec3 v_colorB;
in float v_blend;
in vec3 v_light;
in vec3 v_selection_id;
flat in int v_selected;
out vec4 outputColor;

#define SELECTION_OUTLINE 1
#include "common.glsl"

// Calculate depth based on the given camera position.
float calcDepth( in vec3 cameraPos ){
    vec2 clipZW = cameraPos.z * u_projectionMat[2].zw + u_projectionMat[3].zw;
    return 0.5 + 0.5 * clipZW.x / clipZW.y;
}

// Inigo Quilez ray-cylinder intersection for finite capped cylinder
vec4 cylIntersect(vec3 ro, vec3 rd, vec3 a, vec3 b, float ra) {
    vec3 ba = b - a;
    vec3 oc = ro - a;
    float baba = dot(ba,ba);
    float bard = dot(ba,rd);
    float baoc = dot(ba,oc);
    float k2 = baba - bard*bard;
    float k1 = baba*dot(oc,rd) - baoc*bard;
    float k0 = baba*dot(oc,oc) - baoc*baoc - ra*ra*baba;
    float h = k1*k1 - k2*k0;
    if( h<0.0 ) return vec4(-1.0); // no intersection
    h = sqrt(h);
    float t = (-k1-h)/k2;
    float y = baoc + t*bard;
    if( y>0.0 && y<baba ) return vec4( t, (oc+t*rd - ba*y/baba)/ra );
    // Check caps
    t = (((y<0.0)?0.0:baba) - baoc)/bard;
    if( abs(k1+k2*t)<h ) return vec4( t, ba*sign(y)/length(ba) );
    return vec4(-1.0); // no intersection
}

void main(){
    // Everything is in modelView (camera/view) space
    // Camera is at origin, ray_target is the fragment position in view space
    vec3 ray_target = w.xyz / w.w;
    
    // Unpack variables - all in modelView space
    vec3 base = base_radius.xyz; // base position in modelView space
    float vRadius = base_radius.w; // radius in modelView space  
    vec3 end = end_b.xyz; // end position in modelView space
    float b = end_b.w;
    
    // Ray setup in view space
    vec3 ray_origin = vec3(0.0); // Camera at origin in view space
    vec3 ortho_ray_direction = vec3(0.0, 0.0, 1.0);
    vec3 persp_ray_direction = normalize(ray_target - ray_origin);
    vec3 ray_direction = mix(persp_ray_direction, ortho_ray_direction, u_ortho);
    
    // Use Inigo Quilez intersection with proper cylinder endpoints
    vec4 intersection = cylIntersect(ray_origin, ray_direction, base, end, vRadius);
    
    // Calculate alpha for antialiasing based on distance to cylinder edge
    float alpha = 1.0;
    vec3 surface_point;
    
    if (intersection.x < 0.0) {
        // Calculate distance to cylinder for edge antialiasing
        vec3 axis = normalize(end - base);
        vec3 to_ray = ray_target - base;
        float axis_proj = dot(to_ray, axis);
        vec3 closest_on_axis = base + clamp(axis_proj, 0.0, length(end - base)) * axis;
        float dist_to_surface = length(ray_target - closest_on_axis);
        
        float edge_width = fwidth(dist_to_surface) * 1.5;
        alpha = 1.0 - smoothstep(vRadius - edge_width, vRadius + edge_width, dist_to_surface);
        
        if (alpha < 0.01) {
            discard; // No intersection and no edge contribution
        }
        
        // For edge pixels, use projected surface point
        surface_point = closest_on_axis + normalize(ray_target - closest_on_axis) * vRadius;
    } else {
        // Calculate intersection point using the nearest intersection
        float t = intersection.x;
        surface_point = ray_origin + t * ray_direction;
    }
    
    // Calculate surface normal using original axis direction for consistency
    vec3 original_axis = normalize(v_pointB - v_pointA);
    vec3 point_on_axis = v_pointA + dot(surface_point - v_pointA, original_axis) * original_axis;
    vec3 surface_normal = normalize(surface_point - point_on_axis);
    
    // Keep normal in view space like ellipsoid renderer
    
    // Calculate proper depth - this is crucial for fixing the dark spots
    gl_FragDepth = calcDepth(surface_point);
    
    // Choose color based on actual intersection point using original endpoints
    // This prevents color flipping when view changes
    float axis_position = dot(surface_point - v_pointA, original_axis);
    float cylinder_length = length(v_pointB - v_pointA);
    float axis_t = axis_position / cylinder_length;
    
    // Sharp transition at halfway point: colorA for first half, colorB for second half
    vec3 vColor = (axis_t < 0.5) ? v_colorA : v_colorB;
    
    // Transform both position and normal to world space like sphere impostor renderer
    vec4 worldSpacePos = u_modelViewMatInv * vec4(surface_point, 1.0);
    vec4 worldSpaceNormal = transpose(u_viewMat) * vec4(surface_normal, 1.0);

    // Use unified shading function
    outputColor = calculateShading(u_renderMode, vColor, u_cameraPosVec, worldSpacePos.xyz, worldSpaceNormal.xyz, alpha, v_selection_id);
    outputColor = applyFog(outputColor, u_depthFogColor, u_depthFogOffset, u_depthFogDensity, gl_FragDepth);
}