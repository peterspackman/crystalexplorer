add_library(cx_io
    "${CMAKE_CURRENT_SOURCE_DIR}/ciffile.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/crystalclear.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/fingerprint_eps.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/genericxyzfile.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/io_utilities.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/load_mesh.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/load_wavefunction.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/load_pair_energy_json.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/orcainput.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/pdbfile.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tinyply.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/wavefrontobjfile.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/xyzfile.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/xtb.cpp"
)

target_link_libraries(
    cx_io PUBLIC 
    cx_core
    cx_graphics
    Qt6::Core Qt6::Gui
    Eigen3::Eigen fmt::fmt
    unordered_dense::unordered_dense
    occ_core
    occ_crystal
)

target_compile_features(cx_io PUBLIC cxx_std_20)

target_include_directories(cx_io PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
