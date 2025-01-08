cmake_minimum_required(VERSION 3.18)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
message(STATUS "CrystalExplorer set cmake module path: ${CMAKE_MODULE_PATH}")

# Versioning details
string(TIMESTAMP _PROJECT_VERSION_MAJOR "%y")
string(TIMESTAMP _PROJECT_VERSION_MINOR "%m")
set(PROJECT_VERSION "${_PROJECT_VERSION_MAJOR}.${_PROJECT_VERSION_MINOR}" CACHE STRING "Project version")
string(TIMESTAMP PROJECT_BUILD_DATE "%Y-%m-%d-%H-%M")

project(CrystalExplorer
        LANGUAGES CXX
        VERSION ${PROJECT_VERSION})

if(NOT DEFINED CPACK_SYSTEM_NAME)
    include(CXSystemName)
    cx_get_system_name(_CX_SYSTEM)
    cx_get_architecture(_CX_ARCH)
    set(CPACK_SYSTEM_NAME "${_CX_SYSTEM}-${_CX_ARCH}")
endif()
message(STATUS "Using CPACK_SYSTEM_NAME=${CPACK_SYSTEM_NAME}")

option(ENABLE_CX_TESTS "Enable testing" OFF)
set(OCC_VERSION "0.6.8" CACHE STRING "OCC version number for download")
set(OCC_PLATFORM "${CPACK_SYSTEM_NAME}" CACHE STRING "OCC platform string for download")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM.cmake")

# Set Git revision
include(GitRevision)
set_git_revision(PROJECT_GIT_REVISION)

message(STATUS "${PROJECT_NAME} version: ${PROJECT_VERSION}-${PROJECT_GIT_REVISION} (build date ${PROJECT_BUILD_DATE})")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL Concurrent OpenGLWidgets DBus Test)
find_package(OpenGL REQUIRED)

qt_standard_project_setup()
add_subdirectory(3rdparty)

include(PackagingConfig)

add_subdirectory(shaders)
add_subdirectory(resources)
add_subdirectory(src)

if("${ENABLE_CX_TESTS}")
    enable_testing()
    include(CTest)
    message(STATUS "${PROJECT_NAME} tests enabled")
    add_subdirectory(tests)
else()
    message(STATUS "${PROJECT_NAME} tests disabled")
endif()

include(PackagingInstallConfig)
