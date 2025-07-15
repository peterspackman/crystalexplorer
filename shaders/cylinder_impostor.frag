#version 330
#include "uniforms.glsl"

in vec3 axis;
in vec4 base_radius;
in vec4 end_b;
in vec3 v_pointA;
in vec3 v_pointB;
in vec3 U;
in vec3 V;
in vec4 w;
in vec3 v_colorA;
in vec3 v_colorB;
in vec3 v_light;
in vec3 v_selection_id;
flat in int v_selected;
out vec4 outputColor;

#define SELECTION_OUTLINE 1
#include "common.glsl"

// Calculate depth based on the given camera position.
float calcDepth(in vec3 cameraPos) {
  vec2 clipZW = cameraPos.z * u_projectionMat[2].zw + u_projectionMat[3].zw;
  return 0.5 + 0.5 * clipZW.x / clipZW.y;
}

// Inigo Quilez ray-cylinder intersection for finite capped cylinder
vec4 cylIntersect(vec3 ro, vec3 rd, vec3 a, vec3 b, float ra) {
  vec3 ba = b - a;
  vec3 oc = ro - a;
  float baba = dot(ba, ba);
  float bard = dot(ba, rd);
  float baoc = dot(ba, oc);
  float k2 = baba - bard * bard;
  float k1 = baba * dot(oc, rd) - baoc * bard;
  float k0 = baba * dot(oc, oc) - baoc * baoc - ra * ra * baba;
  float h = k1 * k1 - k2 * k0;
  if (h < 0.0)
    return vec4(-1.0); // no intersection
  h = sqrt(h);
  float t = (-k1 - h) / k2;
  float y = baoc + t * bard;
  if (y > 0.0 && y < baba)
    return vec4(t, (oc + t * rd - ba * y / baba) / ra);
  // Check caps
  t = (((y < 0.0) ? 0.0 : baba) - baoc) / bard;
  if (abs(k1 + k2 * t) < h)
    return vec4(t, ba * sign(y) / length(ba));
  return vec4(-1.0); // no intersection
}

// Determines the surface point, alpha, and depth.
// Returns false if the fragment should be discarded.
bool getSurfaceProperties(vec3 ray_origin, vec3 ray_direction, vec3 base,
                          vec3 end, float radius, vec3 ray_target,
                          out vec3 surface_point, out float alpha) {
  vec4 intersection =
      cylIntersect(ray_origin, ray_direction, base, end, radius);

  if (intersection.x < 0.0) {
    // Edge anti aliasing routine
    // This distance calculation correctly finds the distance to the 2D
    // projection of the *capped* cylinder, thanks to the clamp() function.
    vec3 axis_vec = normalize(end - base);
    vec3 to_ray = ray_target - base;
    float axis_proj = dot(to_ray, axis_vec);
    vec3 closest_on_axis =
        base + clamp(axis_proj, 0.0, length(end - base)) * axis_vec;
    float dist_to_surface = length(ray_target - closest_on_axis);

    float edge_width = fwidth(dist_to_surface) * 1.5;
    alpha = 1.0 - smoothstep(radius - edge_width, radius + edge_width,
                             dist_to_surface);

    if (alpha < 0.01) {
      discard;
      return false;
    }

    // Use the quad's position for lighting calculations.
    surface_point = ray_target;

    // Use the impostor quad's depth instead of pushing to the far plane.
    // This provides a stable depth for the blended edges of the caps.
    gl_FragDepth = calcDepth(ray_target);

  } else {
    // solid interior
    alpha = 1.0;
    surface_point = ray_origin + intersection.x * ray_direction;
    gl_FragDepth = calcDepth(surface_point);
  }
  return true;
}

// Calculates the surface normal in view space.
vec3 getSurfaceNormal(vec3 surface_point, vec3 pA, vec3 pB) {
  vec3 original_axis = normalize(pB - pA);
  vec3 point_on_axis =
      pA + dot(surface_point - pA, original_axis) * original_axis;
  return normalize(surface_point - point_on_axis);
}

// Determines the vertex color based on the position along the cylinder's axis.
vec3 getSurfaceColor(vec3 surface_point, vec3 pA, vec3 pB, vec3 colorA,
                     vec3 colorB) {
  vec3 original_axis = normalize(pB - pA);
  float axis_position = dot(surface_point - pA, original_axis);
  float cylinder_length = length(pB - pA);
  float t = axis_position / cylinder_length;

  // Sharp transition at halfway point
  return (t < 0.5) ? colorA : colorB;
}

void main() {
  vec3 ray_target = w.xyz / w.w;
  vec3 base = base_radius.xyz;
  float vRadius = base_radius.w;
  vec3 end = end_b.xyz;

  vec3 ray_origin = vec3(0.0);
  vec3 persp_ray_direction = normalize(ray_target - ray_origin);
  vec3 ray_direction = mix(persp_ray_direction, vec3(0.0, 0.0, 1.0), u_ortho);

  vec3 surface_point;
  float alpha;
  if (!getSurfaceProperties(ray_origin, ray_direction, base, end, vRadius,
                            ray_target, surface_point, alpha)) {
    return; // Fragment was discarded
  }

  vec3 surface_normal = getSurfaceNormal(surface_point, v_pointA, v_pointB);
  vec3 vColor =
      getSurfaceColor(surface_point, v_pointA, v_pointB, v_colorA, v_colorB);

  vec4 worldSpacePos = u_modelViewMatInv * vec4(surface_point, 1.0);
  vec4 worldSpaceNormal = transpose(u_viewMat) * vec4(surface_normal, 1.0);

  outputColor =
      calculateShading(u_renderMode, vColor, u_cameraPosVec, worldSpacePos.xyz,
                       worldSpaceNormal.xyz, alpha, v_selection_id);
  outputColor = applyFog(outputColor, u_depthFogColor, u_depthFogOffset,
                         u_depthFogDensity, gl_FragDepth);
}
