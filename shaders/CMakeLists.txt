set(SHADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/cylinder.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/cylinder.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/cylinder_impostor.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/cylinder_impostor.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/flag.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/line.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/line.geom
    ${CMAKE_CURRENT_SOURCE_DIR}/line.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/mesh.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/mesh.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/point.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/point.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/simple.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/simple.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/sphere_impostor.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/sphere_impostor.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/triangle.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/triangle.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/colormap.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/uniforms.glsl
    PARENT_SCOPE
)

set(SHADERS_QRC "${CMAKE_CURRENT_SOURCE_DIR}/shaders.qrc" PARENT_SCOPE)
