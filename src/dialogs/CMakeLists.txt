
qt_add_library(cx_dialogs STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/energycalculationdialog.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/energycalculationdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/fragmentstatedialog.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/fragmentstatedialog.ui"
)

target_link_libraries(
    cx_dialogs PUBLIC 
    cx_core
    Qt6::Widgets
)

target_include_directories(cx_dialogs
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
)
target_compile_features(cx_dialogs PUBLIC cxx_std_17)
