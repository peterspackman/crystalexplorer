add_library(cx_graphics
    "${CMAKE_CURRENT_SOURCE_DIR}/billboardrenderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/chemicalstructurerenderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/circlerenderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/colorbarwidget.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/cylinderimpostorrenderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/cylinderrenderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/debugrenderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ellipsoidrenderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/frameworkrenderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/graphics.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/linerenderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/measurement.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/measurementrenderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/meshrenderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/meshinstancerenderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/orbitcamera.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/orientation.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/pointcloudinstancerenderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/pointcloudrenderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/planerenderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/renderselection.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/selection_information.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/scene.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaderloader.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/sphereimpostorrenderer.cpp"
)

target_link_libraries(
    cx_graphics PUBLIC
    cx_core
    cx_crystal
    Qt6::Core Qt6::OpenGL Qt6::Widgets
)

# Link system OpenGL only for non-WASM builds
if(NOT CPACK_SYSTEM_NAME STREQUAL "wasm")
    target_link_libraries(cx_graphics PUBLIC OpenGL::GL)
endif()

target_compile_features(cx_graphics PUBLIC cxx_std_20)

target_include_directories(cx_graphics PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
