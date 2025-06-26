set(CPACK_GENERATOR DragNDrop
    CACHE STRING "Generator for packaging")

set(MACOSX_BUNDLE_ICON_FILE CrystalExplorer.icns)
set(APP_ICON_MACOS ${PROJECT_SOURCE_DIR}/icons/CrystalExplorer.icns)
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

# Code signing configuration
option(ENABLE_CODESIGNING "Enable code signing for distribution" OFF)
set(CODESIGN_IDENTITY "" CACHE STRING "Code signing identity (leave empty for ad-hoc)")
set(CODESIGN_ENTITLEMENTS "" CACHE FILEPATH "Path to entitlements file")

# Auto-detect code signing from environment
if(DEFINED ENV{APPLE_DISTRIBUTION_CERT} AND NOT ENABLE_CODESIGNING)
    set(ENABLE_CODESIGNING ON CACHE BOOL "Enable code signing for distribution" FORCE)
    if(NOT CODESIGN_IDENTITY)
        set(CODESIGN_IDENTITY "$ENV{APPLE_DISTRIBUTION_CERT}" CACHE STRING "Code signing identity" FORCE)
    endif()
    message(STATUS "Auto-enabled code signing with identity: ${CODESIGN_IDENTITY}")
endif()

find_program(MACDEPLOYQT_COMMAND NAMES macdeployqt HINTS "${CMAKE_PREFIX_PATH}/bin")
message(STATUS "macdeployqt located at ${MACDEPLOYQT_COMMAND}")

if(ENABLE_CODESIGNING)
    message(STATUS "Code signing enabled with identity: ${CODESIGN_IDENTITY}")
    if(CODESIGN_ENTITLEMENTS)
        message(STATUS "Using entitlements file: ${CODESIGN_ENTITLEMENTS}")
    endif()
else()
    message(STATUS "Code signing disabled - using ad-hoc signing")
endif()

install(FILES ${APP_ICON_MACOS} DESTINATION ${RESOURCES_DESTINATION}
    COMPONENT Runtime)
