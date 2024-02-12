set(TONTO_EXE ${CMAKE_CURRENT_SOURCE_DIR}/tonto)
set(TONTO_ARCHIVE_STRING "tonto-gfortran-x86_64-linux.tar.gz")
if(WIN32)
    set(TONTO_EXE "${TONTO_EXE}.exe")
    set(TONTO_ARCHIVE_STRING "tonto-mingw-w64-x86_64-windows.tar.gz")
elseif(APPLE)
    set(TONTO_ARCHIVE_STRING "tonto-gfortran-x86_64-osx.tar.gz")
endif()

set(TONTO_VERSION "v21a"
    CACHE STRING "Tonto version string (will be used to fetch from github releases).")
set(TONTO_URL "https://github.com/dylan-jayatilaka/tonto/releases/download/${TONTO_VERSION}/${TONTO_ARCHIVE_STRING}")
if(NOT EXISTS "${TONTO_EXE}")
    message(STATUS "Downloading tonto release from ${TONTO_URL}")
    set(destination "${CMAKE_CURRENT_SOURCE_DIR}/${TONTO_ARCHIVE_STRING}")
    file(DOWNLOAD "${TONTO_URL}" "${destination}"
        SHOW_PROGRESS
        STATUS status
        LOG log)
    list(GET status 0 error_code)
    if(error_code )
        file(REMOVE ${destination})
        # If the file cannot be found on the server,
        # we assume it isn't supposed to have it.
        # We fail with all other errors.
        if( NOT error_code EQUAL 19 AND     # FTP: Invalid file / file not found
            NOT error_code EQUAL 22         # HTTP: HTTP response code said error
        )
            list( GET status 1 error_message )
            message(FATAL_ERROR "Failed to download file tonto HTTP/FTP: ${error_message}" )
        endif()
    endif()
    message(STATUS "Extracting")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xvf "${TONTO_ARCHIVE_STRING}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    message(STATUS "Done extracting tonto")
endif()
set(DESKTOP ${CMAKE_CURRENT_SOURCE_DIR}/crystalexplorer.desktop)
set(BASIS_SETS ${CMAKE_CURRENT_SOURCE_DIR}/basis_sets)
set(ELEMENT_DATA ${CMAKE_CURRENT_SOURCE_DIR}/elementdata.txt)

message(STATUS "elementdata.txt will be installed in ${RESOURCES_DESTINATION}")
install(FILES ${ELEMENT_DATA} DESTINATION ${RESOURCES_DESTINATION}
    COMPONENT Runtime)
message(STATUS "tonto will be installed in ${TONTO_DESTINATION}")
install(PROGRAMS ${TONTO_EXE} DESTINATION ${TONTO_DESTINATION}
    COMPONENT Runtime)
message(STATUS "basis_sets will be installed in ${RESOURCES_DESTINATION}")
install(DIRECTORY ${BASIS_SETS} DESTINATION ${RESOURCES_DESTINATION}
    COMPONENT Runtime)


set(MESH_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/icosphere.obj
    PARENT_SCOPE
)

set(MESH_QRC "${CMAKE_CURRENT_SOURCE_DIR}/mesh.qrc" PARENT_SCOPE)
