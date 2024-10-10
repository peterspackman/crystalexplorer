add_library(cx_core
    "${CMAKE_CURRENT_SOURCE_DIR}/adp.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/atomflags.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/chemicalstructure.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/close_contact_criteria.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/colormap.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/dynamicstructure.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/element.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/elementdata.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/fragment.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/fragment_index.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/frameworkoptions.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/generic_atom_index.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/globalconfiguration.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/hbond_criteria.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/infotable.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/isosurface.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/isosurface_parameters.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mesh.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/meshinstance.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/meshpropertymodel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/molecular_wavefunction.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/object_tree_model.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/pair_energy_parameters.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/pair_energy_results.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/serializable.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/settings.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/slab_options.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/volume.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/wavefunction_parameters.cpp"
)

target_link_libraries(
    cx_core PUBLIC 
    Qt6::Core Qt6::Gui Qt6::Concurrent
    Eigen3::Eigen fmt::fmt
    occ_core
    occ_crystal
    nlohmann_json::nlohmann_json
)

target_compile_features(cx_core PUBLIC cxx_std_20)
target_compile_definitions(cx_core PUBLIC GL_SILENCE_DEPRECATION)

target_include_directories(cx_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
