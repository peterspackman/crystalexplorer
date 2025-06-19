add_library(cx_crystal
    "${CMAKE_CURRENT_SOURCE_DIR}/crystal_json.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/crystalplanegenerator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/crystalplane_unified.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/crystalstructure.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/crystalsurface.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/periodicstructure.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/slabstructure.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/surface_cut_generator.cpp"
)

target_link_libraries(
    cx_crystal PUBLIC 
    cx_core
    occ_crystal 
)

target_compile_features(cx_crystal PUBLIC cxx_std_20)

target_include_directories(cx_crystal PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
