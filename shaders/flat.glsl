

vec3 flatWithNormalOutline(vec3 cameraPos, vec3 position, vec3 normal, vec3 color) {

    vec3 V = normalize(cameraPos - position);
    vec3 N = normalize(normal);
    float c = dot(N, V);
 #ifdef SELECTION_OUTLINE
     if(v_selected == 1) {
         // could use c to mix as well
         color = mix(vec3(u_selectionColor), color, smoothstep(0.0, 0.5, c*c));
     }
     else {
         color = mix(vec3(0), color, smoothstep(-0.5, 0.5, c));
     }
 #endif
     return color;
}
