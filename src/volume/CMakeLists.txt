add_library(cx_volume
    "${CMAKE_CURRENT_SOURCE_DIR}/isosurface_calculator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/volume.cpp"
)

target_link_libraries(
    cx_volume PUBLIC 
    cx_exe
    cx_core
    cx_io
    cx_crystal
)

target_compile_features(cx_volume PUBLIC cxx_std_20)

target_include_directories(cx_volume PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
