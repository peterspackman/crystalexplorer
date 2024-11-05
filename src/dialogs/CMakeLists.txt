# Wrap the UI files
qt_wrap_ui(DIALOG_UI_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/animationsettingsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/celllimitsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/closecontactsdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/depthfadingandclippingdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/elementeditor.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/exportdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/energycalculationdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/fingerprintoptions.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/fileeditor.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/fragmentstatedialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/infoviewer.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/packingdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/periodictabledialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/planegenerationdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/surfacegenerationdialog.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/wavefunctioncalculationdialog.ui"
)

qt_add_library(cx_dialogs STATIC
    ${DIALOG_UI_HEADERS}
    "${CMAKE_CURRENT_SOURCE_DIR}/animationsettingsdialog.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/atominfodocument.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/celllimitsdialog.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/closecontactcriteriawidget.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/closecontactsdialog.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/colordelegate.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/collapsiblegroupbox.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/crystalinfodocument.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/depthfadingandclippingdialog.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/elementeditor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/energycalculationdialog.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/exportdialog.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/fingerprintoptions.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/fingerprintplot.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/fingerprintwindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/fileeditor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/fragmentstatedialog.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/infoviewer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/interactioninfodocument.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/packingdialog.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/periodictabledialog.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/planegenerationdialog.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/surfacedropdown.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/surfacegenerationdialog.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/surfaceinfodocument.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/wavefunctioncalculationdialog.cpp"
)

target_link_libraries(
    cx_dialogs PUBLIC 
    cx_core
    cx_crystal
    cx_io
    cx_graphics
    Qt6::Widgets
)


target_include_directories(cx_dialogs
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
)
target_compile_features(cx_dialogs PUBLIC cxx_std_17)
