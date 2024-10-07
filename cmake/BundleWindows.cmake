set(CPACK_GENERATOR NSIS
    CACHE STRING "Generator for packaging")
set(RUNTIME_DESTINATION "/")
set(WINDOWS_ICON_FILE "${PROJECT_SOURCE_DIR}/icons/CrystalExplorer.ico")
set(OS_BUNDLE WIN32)
# Windows NSIS installer stuff
set(RESOURCES_DESTINATION "/")
set(OCC_DESTINATION "/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/${RUNTIME_DESTINATION}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/${RUNTIME_DESTINATION}")
set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${RUNTIME_DESTINATION}")
set(APPS "\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.exe")
include(InstallRequiredSystemLibraries)
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)

# Enable start menu shortcut creation
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_MODIFY_PATH ON)

# Set the name for your shortcut
set(CPACK_NSIS_DISPLAY_NAME "CrystalExplorer")

# Create a start menu shortcut
set(CPACK_NSIS_MENU_LINKS
  "${PROJECT_NAME}.exe" "CrystalExplorer"
)

# Optional: Set an icon for the shortcut
set(CPACK_NSIS_MUI_ICON "${WINDOWS_ICON_FILE}")
set(CPACK_NSIS_MUI_UNIICON "${WINDOWS_ICON_FILE}")

# Optional: Create a desktop shortcut
set(CPACK_CREATE_DESKTOP_LINKS "CrystalExplorer")
