# Use WiX Toolset to generate MSI installer
set(CPACK_GENERATOR WIX
    CACHE STRING "Generator for packaging")

set(RUNTIME_DESTINATION "/")
set(WINDOWS_ICON_FILE "${PROJECT_SOURCE_DIR}/icons/CrystalExplorer.ico")
set(OS_BUNDLE WIN32)

set(RESOURCES_DESTINATION "/")
set(OCC_DESTINATION "/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/${RUNTIME_DESTINATION}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/${RUNTIME_DESTINATION}")
set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${RUNTIME_DESTINATION}")
set(APPS "\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.exe")

include(InstallRequiredSystemLibraries)
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)

# WiX-specific configuration
set(CPACK_WIX_UPGRADE_GUID "7E8A9B6C-5D4E-4F3A-9B2C-1D8E7F6A5B4C")
set(CPACK_WIX_PRODUCT_ICON "${WINDOWS_ICON_FILE}")
set(CPACK_WIX_PROPERTY_ARPHELPLINK "https://crystalexplorer.net")
set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "https://crystalexplorer.net")

# Create shortcuts in Start Menu and Desktop
set(CPACK_WIX_PROGRAM_MENU_FOLDER "CrystalExplorer")
set(CPACK_PACKAGE_EXECUTABLES "${PROJECT_NAME}" "CrystalExplorer")

# Enable upgrade behavior (uninstall previous versions)
set(CPACK_WIX_SKIP_PROGRAM_FOLDER FALSE)

# UI customization (optional - will use defaults if not present)
if(EXISTS "${PROJECT_SOURCE_DIR}/icons/wix_banner.bmp")
    set(CPACK_WIX_UI_BANNER "${PROJECT_SOURCE_DIR}/icons/wix_banner.bmp")
endif()
if(EXISTS "${PROJECT_SOURCE_DIR}/icons/wix_dialog.bmp")
    set(CPACK_WIX_UI_DIALOG "${PROJECT_SOURCE_DIR}/icons/wix_dialog.bmp")
endif()

# License file for WiX (requires RTF or TXT format)
# Convert COPYING.LESSER to LICENSE.txt for WiX compatibility
if(EXISTS "${PROJECT_SOURCE_DIR}/COPYING.LESSER")
    configure_file(
        "${PROJECT_SOURCE_DIR}/COPYING.LESSER"
        "${CMAKE_BINARY_DIR}/LICENSE.txt"
        COPYONLY
    )
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_BINARY_DIR}/LICENSE.txt")
    message(STATUS "Using COPYING.LESSER as LICENSE.txt for WiX installer")
elseif(EXISTS "${PROJECT_SOURCE_DIR}/LICENSE.rtf")
    set(CPACK_WIX_LICENSE_RTF "${PROJECT_SOURCE_DIR}/LICENSE.rtf")
    message(STATUS "Using LICENSE.rtf for WiX installer")
else()
    message(STATUS "No license file found - WiX will use default EULA")
endif()
