set(XDG_DESKTOP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/crystalexplorer.desktop PARENT_SCOPE)
set(MESH_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/icosphere.obj
    PARENT_SCOPE
)
set(MESH_QRC "${CMAKE_CURRENT_SOURCE_DIR}/mesh.qrc" PARENT_SCOPE)

# OCC version and archive setup
set(OCC_VERSION "0.5.9")
if(WIN32)
    set(OCC_ARCHIVE_STRING "occ-${OCC_VERSION}-windows-x86_64.tar.xz")
elseif(APPLE)
    set(OCC_ARCHIVE_STRING "occ-${OCC_VERSION}-macos-arm64.tar.xz")
else()
    set(OCC_ARCHIVE_STRING "occ-${OCC_VERSION}-linux-x86_64-static.tar.xz")
endif()

# Set up OCC URL and download
set(OCC_URL "https://github.com/peterspackman/occ/releases/download/v${OCC_VERSION}/${OCC_ARCHIVE_STRING}")
message(STATUS "Attempting to download OCC from ${OCC_URL}")

FetchContent_Declare(
    OCC
    URL ${OCC_URL}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_GetProperties(OCC)
if(NOT OCC_POPULATED)
    FetchContent_MakeAvailable(OCC)
endif()

# Set paths based on the extracted content
set(OCC_EXE ${occ_SOURCE_DIR}/bin/occ${CMAKE_EXECUTABLE_SUFFIX})
set(OCC_SHARE_DIRECTORY ${occ_SOURCE_DIR}/share)

# Check if the executable exists and print appropriate message
if(EXISTS ${OCC_EXE})
    message(STATUS "OCC executable successfully downloaded and found at: ${OCC_EXE}")
else()
    message(WARNING "OCC executable not found at expected location: ${OCC_EXE}")
endif()

# Check if the share directory exists and print appropriate message
if(IS_DIRECTORY ${OCC_SHARE_DIRECTORY})
    message(STATUS "OCC share directory found at: ${OCC_SHARE_DIRECTORY}")
else()
    message(WARNING "OCC share directory not found at expected location: ${OCC_SHARE_DIRECTORY}")
endif()

# Installation commands
install(PROGRAMS ${OCC_EXE} 
    DESTINATION ${OCC_DESTINATION}
    COMPONENT Runtime
)
install(DIRECTORY ${OCC_SHARE_DIRECTORY} 
    DESTINATION ${RESOURCES_DESTINATION}
    COMPONENT Runtime
)

message(STATUS "OCC executable will be installed in ${OCC_DESTINATION}")
message(STATUS "OCC data will be installed in ${RESOURCES_DESTINATION}")
