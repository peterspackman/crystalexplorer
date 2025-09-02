add_library(occ_core
    "${CMAKE_CURRENT_SOURCE_DIR}/combinations.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/dimer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/eem.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/elastic_tensor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/element.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/fraction.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/kabsch.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/inertia_tensor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/molecule.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/point_group.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/quasirandom.cpp"
    ${OCC_CORE_INCLUDE_FILES}
)

if("${USE_SYSTEM_BLAS}")
    target_link_libraries(occ_core
	PUBLIC
	${BLAS_LIBRARIES})
    target_compile_definitions(occ_core PUBLIC EIGEN_USE_BLAS)
endif()

target_link_libraries(occ_core
    PUBLIC 
    Eigen3::Eigen 
    fmt::fmt
    unordered_dense::unordered_dense
)
target_include_directories(occ_core PUBLIC 
    ${OCC_INCLUDE_DIR} 
)

target_compile_features(occ_core PUBLIC cxx_std_17)

