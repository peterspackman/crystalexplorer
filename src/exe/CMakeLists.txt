add_library(cx_exe
    "${CMAKE_CURRENT_SOURCE_DIR}/externalprogram.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/occ_external.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/task.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/taskmanager.cpp"
)

target_link_libraries(
    cx_exe PUBLIC 
    unordered_dense::unordered_dense
    occ_core
    cx_core
    Qt6::Core
    Qt6::Concurrent
)

target_compile_features(cx_exe PUBLIC cxx_std_20)

target_include_directories(cx_exe PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)


add_executable(
    test-external-programs
    EXCLUDE_FROM_ALL
    "${CMAKE_CURRENT_SOURCE_DIR}/test_external_programs.cpp"
)
target_link_libraries(test-external-programs
    PUBLIC
    cx_exe
)
