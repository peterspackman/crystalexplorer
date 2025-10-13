set(ICONS_FOLDER ${CMAKE_SOURCE_DIR}/icons)
set(QRC_FOLDER ${ICONS_FOLDER})
set(ICON_NAME CrystalExplorer)

set(LICENSE_FILE "${PROJECT_SOURCE_DIR}/COPYING.LESSER")

set(COMPANY "CrystalExplorer")
set(COPYRIGHT
    "Copyright (c) 2005-2024 Peter R. Spackman, M.J. Turner, "
    "S.K. Wolff, D.J. Grimwood, J.J. McKinnon, "
    "M.J. Turner, D. Jayatilaka, M.A. Spackman. All rights reserved."
)

# Packaging details
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Crystal structure analysis with Hirshfeld surfaces, intermolecular interaction energies and more")
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_CONTACT "Peter Spackman <peterspackman@fastmail.com>")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
# Note: CPACK_RESOURCE_FILE_LICENSE is not set here for WiX compatibility
# WiX requires RTF or TXT format. See BundleWindows.cmake for WiX-specific license handling
if(NOT WIN32)
    set(CPACK_RESOURCE_FILE_LICENSE ${LICENSE_FILE})
endif()
set(CPACK_STRIP_FILES ON)

if(CMAKE_CONFIGURATION_TYPES)
    set(OUTPUT_PREFIX "${OUTPUT_PREFIX}/$<CONFIG>")
endif()

if(APPLE)
    message(STATUS "Packaging for MacOS (defaults to using disk image)")
    include(BundleMacOS)
elseif(UNIX)
    message(STATUS "Packaging for Linux")
    include(BundleLinux)
elseif(WIN32)
    message(STATUS "Packaging for Windows (using WiX for MSI installer)")
    include(BundleWindows)
endif(APPLE)

