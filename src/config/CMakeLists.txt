add_library(cx_config
    INTERFACE
)

target_compile_features(cx_config INTERFACE cxx_std_20)

target_include_directories(cx_config INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
