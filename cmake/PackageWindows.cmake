set(CONTENTS_DIR "${CMAKE_INSTALL_PREFIX}")
set(CX_BINARY_LOCATION "${APPS}")

# Find windeployqt
get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

install(CODE "
    message(STATUS \"Bundling Qt libraries into ${CONTENTS_DIR} using windeployqt\")
    message(STATUS \"CX_BINARY_LOCATION ${CX_BINARY_LOCATION}\")

    execute_process(
        COMMAND \"${CMAKE_COMMAND}\" -E env PATH=\"${_qt_bin_dir}\"
        \"${WINDEPLOYQT_EXECUTABLE}\"
            --verbose 1
            --compiler-runtime
            --no-translations
            --no-system-d3d-compiler
            --no-opengl-sw
            \"${CX_BINARY_LOCATION}\"
        RESULT_VARIABLE WINDEPLOYQT_RESULT
    )

    if(NOT WINDEPLOYQT_RESULT EQUAL 0)
        message(FATAL_ERROR \"windeployqt failed with exit code \${WINDEPLOYQT_RESULT}\")
    endif()
" COMPONENT Runtime)
