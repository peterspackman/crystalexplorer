add_library(occ_crystal
    "${CMAKE_CURRENT_SOURCE_DIR}/asymmetric_unit.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/crystal.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/dimer_mapping_table.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/hkl.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/spacegroup.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/surface.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/symmetryoperation.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/unitcell.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/unit_cell_connectivity.cpp"
    ${occ_INCLUDE_FILES}
)

target_link_libraries(occ_crystal
    PUBLIC 
    occ_core
    gemmi::gemmi_cpp
)
target_include_directories(occ_crystal PUBLIC 
    ${OCC_INCLUDE_DIR} 
)

target_compile_features(occ_crystal PUBLIC cxx_std_17)
