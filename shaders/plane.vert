#version 330

// Per-vertex attributes
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;

// Per-instance attributes
layout(location = 2) in vec3 origin;
layout(location = 3) in vec3 axisA;
layout(location = 4) in vec3 axisB;
layout(location = 5) in vec4 color;
layout(location = 6) in vec4
    gridParams; // [showGrid, gridSpacing, showAxes, showBounds]
layout(location = 7) in vec4 boundsA; // [minA, maxA, unused, unused]
layout(location = 8) in vec4 boundsB; // [minB, maxB, unused, unused]

// Uniforms
uniform mat4 u_modelViewProjectionMat;
uniform mat4 u_viewMat;
uniform mat4 u_projectionMat;
uniform float u_scale;

// Outputs
out vec4 v_color;
out vec2 v_texcoord;
out vec2 v_scaledCoord; // Coordinates scaled by bounds for grid
out vec3 v_worldPos;
out vec4 v_gridParams;
out vec4 v_boundsA;
out vec4 v_boundsB;
out vec3 v_axisA;
out vec3 v_axisB;

void main() {
  // Scale the unit quad (0,0 to 1,1) to the bounds range
  float scaledA = mix(boundsA.x, boundsA.y, position.x);
  float scaledB = mix(boundsB.x, boundsB.y, position.y);

  // Transform using the scaled coordinates and instance data
  vec3 worldPos = origin + scaledA * axisA + scaledB * axisB;

  gl_Position = u_modelViewProjectionMat * vec4(worldPos, 1.0);

  // Pass through data to fragment shader
  v_color = color;
  v_texcoord = texcoord;
  v_scaledCoord = vec2(scaledA, scaledB); // Scaled coordinates for grid
  v_worldPos = worldPos;
  v_gridParams = gridParams;
  v_boundsA = boundsA;
  v_boundsB = boundsB;
  v_axisA = axisA;
  v_axisB = axisB;
}
