set(CONTENTS_DIR "${APPS}/Contents")
install(CODE "
    message(\"Bundling QT libraries into ${APPS} using macdeployqt\")
    message(\"CX_BINARY_LOCATION ${CX_BINARY_LOCATION}\")
    execute_process(COMMAND ${MACDEPLOYQT_COMMAND} ${APPS} -executable=${CX_BINARY_LOCATION})
    message(STATUS \"Running ad hoc code signing\")
    execute_process(COMMAND
	codesign --force --deep --sign - ${CONTENTS_DIR}/MacOS/CrystalExplorer
	)
    execute_process(COMMAND
	sh -c \
	\"find ${CONTENTS_DIR}/MacOS ${CONTENTS_DIR}/Frameworks \
	${CONTENTS_DIR}/Resources ${CONTENTS_DIR}/Plugins \
	\\\\( \
	-name \\\"*.dylib\\\" \
	-o -name \\\"*.framework\\\" \
	-o -name \\\"tonto\\\" \
	-maxdepth 1 \
	\\\\) \
	-exec codesign --force --sign - {} \\\;\"
	)
    execute_process(COMMAND
	codesign --force --deep --sign - ${APPS}
    )
" COMPONENT Runtime)
