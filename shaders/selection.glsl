vec3 id_to_color(uint id) {
    // Assuming ID is packed in 24-bits for RGB
    float r = float((id >> uint(16)) & uint(0xFF)) / 255.0;
    float g = float((id >> uint(8)) & uint(0xFF)) / 255.0;
    float b = float(id & uint(0xFF)) / 255.0;
    return vec3(r, g, b);
}

uint color_to_id(vec3 color) {
    uint r = uint(color.r * 255.0);
    uint g = uint(color.g * 255.0);
    uint b = uint(color.b * 255.0);
    return (r << 16) | (g << 8) | b;
}
