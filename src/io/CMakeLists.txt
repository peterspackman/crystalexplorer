add_library(cx_io
    "${CMAKE_CURRENT_SOURCE_DIR}/genericxyzfile.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/wavefrontobjfile.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/xyzfile.cpp"
)

target_link_libraries(
    cx_io PUBLIC 
    cx_core
    Qt6::Core Qt6::Gui
    Eigen3::Eigen sbf_cpp fmt::fmt
    unordered_dense::unordered_dense
    occ_core
)

target_compile_features(cx_io PUBLIC cxx_std_20)

target_include_directories(cx_io PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
