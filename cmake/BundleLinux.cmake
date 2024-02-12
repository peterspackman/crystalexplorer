set(CPACK_GENERATOR DEB
    CACHE STRING "Generator for packaging")
set(CMAKE_INSTALL_PREFIX usr)
# DEB packaging
set(plugin_dest_dir bin)
set(qtconf_dest_dir bin)
set(RUNTIME_DESTINATION "bin")
set(TONTO_DESTINATION ${RUNTIME_DESTINATION})
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(RESOURCES_DESTINATION "share/crystalexplorer")
set(TONTO_DESTINATION ${RUNTIME_DESTINATION})
set(APPS "\${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME}")

set(ICON_FILE ${ICONS_FOLDER}/${ICON_NAME}.png)
set(DESKTOP resources/crystalexplorer.desktop)
# Free desktop stuff
install(FILES ${DESKTOP} DESTINATION share/applications
    COMPONENT Runtime)
install(FILES ${ICON_FILE} DESTINATION share/pixmaps
    COMPONENT Runtime)

