add_subdirectory(occ)
add_subdirectory(config)
add_subdirectory(core)
add_subdirectory(exe)
add_subdirectory(io)
add_subdirectory(crystal)
add_subdirectory(graphics)
add_subdirectory(pair_energy)
add_subdirectory(volume)

# Source files
set(CRYSTALEXPLORER_SOURCE_FILES
    animationsettingsdialog.cpp
    asymmetricunit.cpp
    atom.cpp
    celllimitsdialog.cpp
    chargedialog.cpp
    childpropertycontroller.cpp
    ciffile.cpp
    closecontactsdialog.cpp
    colordelegate.cpp
    colorschemer.cpp
    confirmationbox.cpp
    deprecatedcrystal.cpp
    crystalcontroller.cpp
    crystaldata.cpp
    crystalplanegenerator.cpp
    crystalsurfacehandler.cpp
    crystalx.cpp
    depthfadingandclippingdialog.cpp
    dialoghtml.cpp
    elementeditor.cpp
    energycalculationdialog.cpp
    energydata.cpp
    fileeditor.cpp
    fingerprintoptions.cpp
    fingerprintplot.cpp
    fingerprintwindow.cpp
    fragmentpairenergyjob.cpp
    frameworkdialog.cpp
    gaussianinterface.cpp
    glwindow.cpp
    gltfexporter.cpp
    infodocuments.cpp
    infoviewer.cpp
    jobparameters.cpp
    measurement.cpp
    nwcheminterface.cpp
    occinterface.cpp
    orbitcamera.cpp
    orcainterface.cpp
    orientation.cpp
    packingdialog.cpp
    planegenerationdialog.cpp
    periodictabledialog.cpp
    preferencesdialog.cpp
    project.cpp
    psi4interface.cpp
    quantumchemistryinterface.cpp
    scene.cpp
    settings.cpp
    spacegroup.cpp
    surface.cpp
    surfacedata.cpp
    surfacedescription.cpp
    surfacedropdown.cpp
    surfacegenerationdialog.cpp
    surfaceproperty.cpp
    tasksequence.cpp
    tontointerface.cpp
    unitcell.cpp
    viewtoolbar.cpp
    wavefunction.cpp
    wavefunctioncalculationdialog.cpp
    xtbinterface.cpp
)

set(CRYSTALEXPLORER_HEADER_FILES
    animationsettingsdialog.h
    asymmetricunit.h
    atom.h
    atomid.h
    cameraoptions.h
    celllimitsdialog.h
    chargedialog.h
    chargemultiplicitypair.h
    childpropertycontroller.h
    ciffile.h
    closecontactsdialog.h
    colordelegate.h
    colorschemer.h
    confirmationbox.h
    deprecatedcrystal.h
    crystalcontroller.h
    crystaldata.h
    crystalplanegenerator.h
    crystalsurfacehandler.h
    crystalx.h
    depthfadingandclippingdialog.h
    dialoghtml.h
    elementeditor.h
    energycalculationdialog.h
    energydata.h
    energydescription.h
    fileeditor.h
    fingerprintoptions.h
    fingerprintplot.h
    fingerprintwindow.h
    fragmentpairenergyjob.h
    fragmentpairinfo.h
    frameworkdescription.h
    frameworkdialog.h
    gaussianinterface.h
    globals.h
    glwindow.h
    gltfexporter.h
    infodocuments.h
    infoviewer.h
    jobparameters.h
    measurement.h
    nwcheminterface.h
    occinterface.h
    orcainterface.h
    orbitcamera.h
    orientation.h
    packingdialog.h
    periodictabledialog.h
    planegenerationdialog.h
    preferencesdialog.h
    project.h
    psi4interface.h
    qeigen.h
    quantumchemistryinterface.h
    scene.h
    settings.h
    spacegroup.h
    stldatastream.h
    surface.h
    surfacedata.h
    surfacedescription.h
    surfacedropdown.h
    surfacegenerationdialog.h
    surfaceproperty.h
    vanderwaalscontact.h
    version.h
    xtbinterface.h
    tasksequence.h
    tontointerface.h
    transformablewavefunction.h
    unitcell.h
    viewtoolbar.h
    wavefunction.h
    wavefunctioncalculationdialog.h
)


set(CRYSTALEXPLORER_UI_FILES 
    animationsettingsdialog.ui
    celllimitsdialog.ui
    chargedialog.ui
    childpropertycontroller.ui
    closecontactsdialog.ui
    crystalcontroller.ui
    crystalx.ui
    depthfadingandclippingdialog.ui
    elementeditor.ui
    energycalculationdialog.ui
    fileeditor.ui
    fingerprintoptions.ui
    frameworkdialog.ui
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
