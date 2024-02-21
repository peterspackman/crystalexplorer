add_library(cx_core
    "${CMAKE_CURRENT_SOURCE_DIR}/chemicalstructure.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/element.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/elementdata.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/interactions.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/isosurface.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mesh.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/volume.cpp"
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
