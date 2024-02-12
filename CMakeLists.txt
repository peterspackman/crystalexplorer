cmake_minimum_required(VERSION 3.18)
enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
message(STATUS "CMake module path: ${CMAKE_MODULE_PATH}")

# Versioning details
set(PROJECT_VERSION_MAJOR 17)
string(TIMESTAMP PROJECT_VERSION_MAJOR "%y")
string(TIMESTAMP PROJECT_VERSION_MINOR "%m")
string(TIMESTAMP PROJECT_VERSION_MINOR_MINOR "%d")
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
string(TIMESTAMP PROJECT_BUILD_DATE "%Y-%m-%d %H:%M")

project(CrystalExplorer
        LANGUAGES CXX
        VERSION ${PROJECT_VERSION})

option(ENABLE_TESTS "Enable testing" ON)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM.cmake")

# Set Git revision
include(GitRevision)
set_git_revision(PROJECT_GIT_REVISION)

message(STATUS "${PROJECT_NAME} v${PROJECT_VERSION}-${PROJECT_GIT_REVISION} (${PROJECT_BUILD_DATE})")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL Concurrent OpenGLWidgets DBus Test)
find_package(OpenGL REQUIRED)

qt_standard_project_setup()
add_subdirectory(3rdparty)

###################################################
############ PACKAGING INFORMATION ################
###################################################

set(ICONS_FOLDER ${CMAKE_SOURCE_DIR}/icons)
set(QRC_FOLDER ${ICONS_FOLDER})
set(ICON_NAME crystalexplorer)

set(LICENSE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.LESSER")

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
set(CPACK_RESOURCE_FILE_LICENSE ${LICENSE_FILE})
set(CPACK_STRIP_FILES ON)

if(CMAKE_CONFIGURATION_TYPES)
    set(OUTPUT_PREFIX "${OUTPUT_PREFIX}/$<CONFIG>")
endif()

if(APPLE)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/BundleMacOS.cmake")
elseif(UNIX)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/BundleLinux.cmake")
elseif(WIN32)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/BundleWindows.cmake")
endif(APPLE)

# Must occur after packaging information for install etc. to work properly
add_subdirectory(shaders)
add_subdirectory(resources)
add_subdirectory(src)

if("${ENABLE_TESTS}")
    include(CTest)
    message(STATUS "${PROJECT_NAME} tests enabled")
    add_subdirectory(tests)
else()
    message(STATUS "${PROJECT_NAME} tests disabled")
endif()


if(APPLE)
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/PackageMacOS.cmake")
endif(APPLE)

include(CPack)
