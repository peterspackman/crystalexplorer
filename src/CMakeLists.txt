add_subdirectory(occ)
add_subdirectory(core)
add_subdirectory(exe)
add_subdirectory(io)
add_subdirectory(crystal)
add_subdirectory(graphics)
add_subdirectory(pair_energy)
add_subdirectory(volume)
add_subdirectory(dialogs)

# Source files
set(CRYSTALEXPLORER_SOURCE_FILES
    animationsettingsdialog.cpp
    celllimitsdialog.cpp
    childpropertycontroller.cpp
    closecontactsdialog.cpp
    colordelegate.cpp
    confirmationbox.cpp
    crystalcontroller.cpp
    crystalplanegenerator.cpp
    crystalx.cpp
    depthfadingandclippingdialog.cpp
    dialoghtml.cpp
    elementeditor.cpp
    fileeditor.cpp
    fingerprintoptions.cpp
    fingerprintplot.cpp
    fingerprintwindow.cpp
    glwindow.cpp
    infodocuments.cpp
    infoviewer.cpp
    measurement.cpp
    orbitcamera.cpp
    orientation.cpp
    packingdialog.cpp
    planegenerationdialog.cpp
    periodictabledialog.cpp
    preferencesdialog.cpp
    project.cpp
    scene.cpp
    spacegroup.cpp
    surfacedropdown.cpp
    surfacegenerationdialog.cpp
    viewtoolbar.cpp
    wavefunctioncalculationdialog.cpp
)

set(CRYSTALEXPLORER_HEADER_FILES
    animationsettingsdialog.h
    cameraoptions.h
    celllimitsdialog.h
    chargemultiplicitypair.h
    childpropertycontroller.h
    closecontactsdialog.h
    colordelegate.h
    confirmationbox.h
    crystalcontroller.h
    crystalplanegenerator.h
    crystalx.h
    depthfadingandclippingdialog.h
    dialoghtml.h
    elementeditor.h
    fileeditor.h
    fingerprintoptions.h
    fingerprintplot.h
    fingerprintwindow.h
    glwindow.h
    infodocuments.h
    infoviewer.h
    measurement.h
    orbitcamera.h
    orientation.h
    packingdialog.h
    periodictabledialog.h
    planegenerationdialog.h
    preferencesdialog.h
    project.h
    qeigen.h
    scene.h
    spacegroup.h
    stldatastream.h
    surfacedropdown.h
    surfacegenerationdialog.h
    viewtoolbar.h
    wavefunctioncalculationdialog.h
)


set(CRYSTALEXPLORER_UI_FILES 
    animationsettingsdialog.ui
    celllimitsdialog.ui
    childpropertycontroller.ui
    closecontactsdialog.ui
    crystalcontroller.ui
    crystalx.ui
    depthfadingandclippingdialog.ui
    elementeditor.ui
    fileeditor.ui
    fingerprintoptions.ui
    infochooserdialog.ui
    infoviewer.ui
    packingdialog.ui
    periodictabledialog.ui
    planegenerationdialog.ui
    preferencesdialog.ui
    surfacegenerationdialog.ui
    wavefunctioncalculationdialog.ui
)


if(WIN32)
    set(CRYSTALEXPLORER_RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/windows_metafile.rc")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/windows_metafile.rc.in"
	"${CRYSTALEXPLORER_RC_FILE}"
    )
endif()

# Set the executable details
qt_add_resources(
    CRYSTALEXPLORER_RESOURCE_FILES
    ${QRC_FOLDER}/crystalexplorer.qrc ${SHADERS_QRC} ${MESH_QRC}
)

qt_add_library(cx STATIC
    ${CRYSTALEXPLORER_SOURCE_FILES}
    ${CRYSTALEXPLORER_HEADER_FILES}
    ${CRYSTALEXPLORER_RC_FILE}
    ${CRYSTALEXPLORER_UI_FILES}
    ${CRYSTALEXPLORER_RESOURCE_FILES}
)
target_link_libraries(
    cx PUBLIC 
    cx_core
    cx_exe
    cx_volume
    cx_io
    cx_crystal
    cx_graphics
    cx_pair_energy
    cx_dialogs
    Qt6::Widgets Qt6::Concurrent
    Qt6::OpenGLWidgets Qt6::DBus
)
target_include_directories(cx
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
)
target_compile_features(cx PUBLIC cxx_std_17)
target_compile_definitions(cx
    PUBLIC
    PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    PROJECT_GIT_REVISION=${PROJECT_GIT_REVISION}
    PROJECT_VERSION=${PROJECT_VERSION}
    PROJECT_BUILD_DATE=${PROJECT_BUILD_DATE}
    HS_PREVIOUS_VERSION="17.5"
    GL_SILENCE_DEPRECATION=1
)

qt_add_executable(
    CrystalExplorer 
    ${OS_BUNDLE}
    ${CRYSTALEXPLORER_RESOURCE_FILES} 
    main.cpp ${CRYSTALEXPLORER_RC_FILE} ${CRYSTALEXPLORER_HEADER_FILES}
    ${APP_ICON_MACOS}
)
target_link_libraries(
  CrystalExplorer PUBLIC cx 
)

set_target_properties(CrystalExplorer PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_GUI_IDENTIFIER net.crystalexplorer.CrystalExplorer
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ""
)

target_include_directories(CrystalExplorer
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
    PUBLIC "${CMAKE_CURRENT_BINARY_DIR}"
)

install(TARGETS CrystalExplorer
    BUNDLE DESTINATION . COMPONENT Runtime
    RUNTIME DESTINATION ${RUNTIME_DESTINATION} COMPONENT Runtime
)
