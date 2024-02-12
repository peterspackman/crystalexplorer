#version 330
uniform mat4 u_view;
uniform mat4 u_model;
uniform mat4 u_modelViewMat;
uniform mat4 u_normalMat;
uniform vec4 u_lightPos;

in highp vec4 v_color;
in highp vec3 v_normal;
in highp vec3 v_position;
out highp vec4 f_color;

//struct Material {
//    float ambient = 0.3;
//    float diffuse = 0.5;
//    float specular = 0.2;
//} u_material;

const vec3 u_material = vec3(0.1, 0.5, 0.4);
const vec4 u_lightIntensity = vec4(1);
const float u_shininess = 10.0;
const float u_screenGamma = 2.2; // Assume the monitor is calibrated to the sRGB color space

vec3 gammaCorrection(vec3 color, float gamma) {
    return pow(color, vec3(1.0/gamma));
}

vec3 lighting(vec3 material, vec3 normal, vec3 lightDirection, vec3 color) {

    float lambert = clamp(dot(normal, lightDirection), 0.0, 1.0);
    float specular = 0.0;

    if (lambert > 0) {
        // Blinn-Phong model
        vec3 viewDirection = normalize(-v_position);
        vec3 halfDir = normalize(lightDirection + viewDirection);
        float specularAngle = max(dot(halfDir, normal), 0.0);
        specular = pow(specularAngle, u_shininess);
    }
    vec3 colorLinear = color * material.x +
                  color * material.y * lambert +
                  color * material.z * specular;

    return gammaCorrection(colorLinear, u_screenGamma);
}



void main()
{
   vec3 normal = normalize(v_normal);
   vec3 lightDirection = normalize(
                u_lightPos - vec3(u_modelViewMat * vec4(v_position, 1.0)));

   f_color = vec4(lighting(u_material, normal, lightDirection, v_color.xyz),
                  v_color.w * v_color.w);
}
