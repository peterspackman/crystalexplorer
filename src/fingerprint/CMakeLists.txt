qt_add_library(cx_fingerprint STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/fingerprintoptions.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/fingerprintoptions.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/fingerprintplot.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/fingerprintwindow.cpp"
)

target_link_libraries(
    cx_fingerprint PUBLIC 
    cx_core
    cx_crystal
    cx_io
    cx_graphics
    Qt6::Widgets
)

target_include_directories(cx_fingerprint
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
)
target_compile_features(cx_fingerprint PUBLIC cxx_std_17)
