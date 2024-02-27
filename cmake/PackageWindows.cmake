set(CONTENTS_DIR "${CMAKE_INSTALL_PREFIX}")
set(CX_BINARY_LOCATION "${APPS}")
install(CODE "
message(\"Bundling QT libraries into ${CONTENTS_DIR} using windeployqt\")
message(\"CX_BINARY_LOCATION ${CX_BINARY_LOCATION}\")
execute_process(COMMAND ${WINDEPLOYQT_COMMAND} ${APPS})
" COMPONENT Runtime)
