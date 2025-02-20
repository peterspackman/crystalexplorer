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
    childpropertycontroller.cpp
    confirmationbox.cpp
    projectcontroller.cpp
    crystalx.cpp
    dialoghtml.cpp
    glwindow.cpp
    preferencesdialog.cpp
    project.cpp
    viewtoolbar.cpp
)

set(CRYSTALEXPLORER_HEADER_FILES
    childpropertycontroller.h
    confirmationbox.h
    projectcontroller.h
    default_paths.h
    crystalx.h
    dialoghtml.h
    glwindow.h
    preferencesdialog.h
    project.h
    viewtoolbar.h
)


qt_wrap_ui(CRYSTALEXPLORER_UI_FILES 
    childpropertycontroller.ui
    crystalcontroller.ui
    crystalx.ui
    preferencesdialog.ui
)


if(WIN32)
    set(CRYSTALEXPLORER_RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/windows_metafile.rc")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/windows_metafile.rc" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    file(COPY ${WINDOWS_ICON_FILE} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
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
    MACOSX_BUNDLE_ICON_FILE "${MACOSX_BUNDLE_ICON_FILE}"
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
