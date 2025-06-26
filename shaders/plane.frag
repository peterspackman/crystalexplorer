#version 330
#include "uniforms.glsl"

#include "common.glsl"

// Inputs from vertex shader
in vec4 v_color;
in vec2 v_texcoord;
in vec2 v_scaledCoord; // Coordinates scaled by bounds for grid
in vec3 v_worldPos;
in vec4 v_gridParams; // [showGrid, gridSpacing, showAxes, showBounds]
in vec4 v_boundsA;    // [minA, maxA, unused, unused]
in vec4 v_boundsB;    // [minB, maxB, unused, unused]
in vec3 v_axisA;
in vec3 v_axisB;

// Additional uniforms (others come from uniforms.glsl)
// Note: u_texture conflicts with uniforms.glsl, so we'll skip texture support
// for now

// Output
out vec4 outputColor;

// Simple grid line rendering
float gridLine(float coord) {
  float grid = abs(fract(coord + 0.5) - 0.5) / fwidth(coord);
  return 1.0 - min(grid, 1.0);
}

void main() {
  vec3 finalColor = v_color.rgb;
  float alpha = v_color.a;

  // Check if grid should be shown
  bool showGrid = v_gridParams.x > 0.5;
  float gridSpacing = v_gridParams.y;
  
  if (showGrid && gridSpacing > 0.0) {
    // Convert grid spacing to texture coordinate scale
    // For spacing of 0.1, we want 10 grid cells per unit texture coordinate
    float gridScale = 1.0 / gridSpacing;
    
    // Create grid pattern by alternating brightness based on grid cells
    // Use scaled coordinates which represent actual plane units
    float gridX = floor(v_scaledCoord.x * gridScale);
    float gridY = floor(v_scaledCoord.y * gridScale);
    float gridPattern = mod(gridX + gridY, 2.0);
    
    // Calculate perceived brightness of the base color
    float brightness = perceivedBrightness(finalColor);
    
    // Alternate between slightly darker and lighter based on grid pattern
    float brightnessAdjust = mix(-0.1, 0.1, gridPattern); // -10% to +10% brightness
    finalColor = finalColor + vec3(brightnessAdjust);
    
    // Clamp to valid range
    finalColor = clamp(finalColor, 0.0, 1.0);
  }

  // Bounds visualization
  bool showBounds = v_gridParams.w > 0.5;
  if (showBounds) {
    float boundU = v_texcoord.x;
    float boundV = v_texcoord.y;

    // Highlight bounds edges
    float edgeWidth = 0.02;
    bool isEdge = (boundU < edgeWidth || boundU > 1.0 - edgeWidth ||
                   boundV < edgeWidth || boundV > 1.0 - edgeWidth);

    if (isEdge) {
      finalColor = mix(finalColor, vec3(1.0, 0.0, 0.0), 0.5);
    }
  }

  // Gamma correction
  vec3 colorGammaCorrected = pow(finalColor, vec3(1.0 / u_screenGamma));
  outputColor = vec4(colorGammaCorrected, alpha);
}
