
vec3 encodeIdToVec3(uint type, uint idOrBaseId, uint vertexId) {
    uint encodedId;
    if (type == 3U) { // Mesh
        idOrBaseId &= (1U << 5U) - 1U; // Keep only 5 bits
        vertexId &= (1U << 16U) - 1U;  // Keep only 16 bits
        encodedId = (type << 21U) | (idOrBaseId << 16U) | vertexId;
    } else { // Atom/Bond
        idOrBaseId &= (1U << 21U) - 1U; // Keep only 21 bits
        encodedId = (type << 21U) | idOrBaseId;
    }

    // Convert the encodedId to vec3, each component in the range [0, 1]
    float r = float((encodedId >> 16U) & 0xFFU) / 255.0;
    float g = float((encodedId >> 8U) & 0xFFU) / 255.0;
    float b = float(encodedId & 0xFFU) / 255.0;
    return vec3(r, g, b);
}

void decodeVec3ToId(vec3 color, out uint type, out uint idOrBaseId, out uint vertexId) {
    uint encodedId = (uint(color.r * 255.0) << 16U) | (uint(color.g * 255.0) << 8U) | uint(color.b * 255.0);
    type = (encodedId >> 21U) & 0x3U;
    
    if (type == 2U) { // Mesh
        idOrBaseId = (encodedId >> 16U) & ((1U << 5U) - 1U);
        vertexId = encodedId & ((1U << 16U) - 1U);
    } else { // Atom/Bond
        idOrBaseId = encodedId & ((1U << 21U) - 1U);
        vertexId = 0U; // Not applicable for Atom/Bond
    }
}

