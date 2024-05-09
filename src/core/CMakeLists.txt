add_library(cx_core
    "${CMAKE_CURRENT_SOURCE_DIR}/chemicalstructure.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/element.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/elementdata.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/fragment.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/generic_atom_index.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/globalconfiguration.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/interactions.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/isosurface.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/isosurface_parameters.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mesh.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/meshinstance.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/meshpropertymodel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/molecular_wavefunction.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/pair_energy_parameters.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/volume.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/wavefunction_parameters.cpp"
)

target_link_libraries(
    cx_core PUBLIC 
    Qt6::Core Qt6::Gui Qt6::Concurrent
    Eigen3::Eigen sbf_cpp fmt::fmt
    occ_core
)

target_compile_features(cx_core PUBLIC cxx_std_20)
target_compile_definitions(cx_core PUBLIC GL_SILENCE_DEPRECATION)

target_include_directories(cx_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
