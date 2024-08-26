set(CPACK_GENERATOR DragNDrop
    CACHE STRING "Generator for packaging")

set(MACOSX_BUNDLE_ICON_FILE ${PROJECT_SOURCE_DIR}/icons/crystalexplorer.icns)
set(APP_ICON_MACOS ${PROJECT_SOURCE_DIR}/icons/crystalexplorer.icns)
set_source_files_properties(${APP_ICON_MACOS} PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources")

set(RUNTIME_DESTINATION "/")
set(OS_BUNDLE MACOSX_BUNDLE)
set(plugin_dest_dir "${PROJECT_NAME}.app/Contents/" )
set(qtconf_dest_dir "${PROJECT_NAME}.app/Contents/Resources")
set(APPS "\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app")
set(CX_BINARY_LOCATION "${APPS}/Contents/MacOS/CrystalExplorer")
set(RESOURCES_DESTINATION "${PROJECT_NAME}.app/Contents/Resources")
set(OCC_DESTINATION "${PROJECT_NAME}.app/Contents/MacOS/")
find_program(MACDEPLOYQT_COMMAND NAMES macdeployqt HINTS "${CMAKE_PREFIX_PATH}/bin")
message(STATUS "macdeployqt located at ${MACDEPLOYQT_COMMAND}")
install(FILES ${META_FILES_TO_INCLUDE} ${APP_ICON_MACOS} DESTINATION ${RESOURCES_DESTINATION}
    COMPONENT Runtime)
