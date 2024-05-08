add_library(cx_pair_energy
    "${CMAKE_CURRENT_SOURCE_DIR}/pair_energy_calculator.cpp"
)

target_link_libraries(
    cx_pair_energy PUBLIC 
    cx_exe
    cx_core
    cx_io
)

target_compile_features(cx_pair_energy PUBLIC cxx_std_20)

target_include_directories(cx_pair_energy PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
