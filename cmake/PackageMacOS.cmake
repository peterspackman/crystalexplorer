set(CONTENTS_DIR "${APPS}/Contents")

install(CODE "
    message(\"Bundling QT libraries into ${APPS} using macdeployqt\")
    message(\"CX_BINARY_LOCATION ${CX_BINARY_LOCATION}\")
    
    # Try macdeployqt with additional options to resolve Qt frameworks
    execute_process(
        COMMAND ${MACDEPLOYQT_COMMAND} ${APPS} 
            -executable=${CX_BINARY_LOCATION}
            -always-overwrite
            -verbose=2
        RESULT_VARIABLE MACDEPLOYQT_RESULT
        OUTPUT_VARIABLE MACDEPLOYQT_OUTPUT
        ERROR_VARIABLE MACDEPLOYQT_ERROR
    )
    
    if(NOT MACDEPLOYQT_RESULT EQUAL 0)
        message(WARNING \"macdeployqt encountered issues but continuing...\")
        message(\"Output: \${MACDEPLOYQT_OUTPUT}\")
        message(\"Error: \${MACDEPLOYQT_ERROR}\")
    endif()
    
    # Determine signing parameters at install time
    if(${ENABLE_CODESIGNING} AND \"${CODESIGN_IDENTITY}\" STREQUAL \"\")
        # Auto-detect certificate if not explicitly set
        execute_process(
            COMMAND security find-identity -v -p codesigning
            COMMAND grep \"Developer ID Application\"
            COMMAND head -1
            OUTPUT_VARIABLE CERT_LINE
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        string(REGEX MATCH \"\\\"([^\\\"]+)\\\"\" CERT_MATCH \"\${CERT_LINE}\")
        if(CERT_MATCH)
            set(DETECTED_IDENTITY \"\${CMAKE_MATCH_1}\")
            message(STATUS \"Auto-detected certificate: \${DETECTED_IDENTITY}\")
        endif()
    endif()
    
    # Set signing commands based on configuration
    if(${ENABLE_CODESIGNING})
        if(NOT \"${CODESIGN_IDENTITY}\" STREQUAL \"\")
            set(SIGN_IDENTITY \"${CODESIGN_IDENTITY}\")
        elseif(DETECTED_IDENTITY)
            set(SIGN_IDENTITY \"\${DETECTED_IDENTITY}\")
        else()
            message(FATAL_ERROR \"Code signing enabled but no certificate identity found\")
        endif()
        
        message(STATUS \"Running distribution code signing with identity: \${SIGN_IDENTITY}\")
        set(SIGN_FLAGS --force --timestamp --options runtime --sign \"\${SIGN_IDENTITY}\")
        set(DEEP_SIGN_FLAGS --force --deep --timestamp --options runtime --sign \"\${SIGN_IDENTITY}\")
        
        if(NOT \"${CODESIGN_ENTITLEMENTS}\" STREQUAL \"\")
            list(APPEND SIGN_FLAGS --entitlements \"${CODESIGN_ENTITLEMENTS}\")
            list(APPEND DEEP_SIGN_FLAGS --entitlements \"${CODESIGN_ENTITLEMENTS}\")
        endif()
    else()
        message(STATUS \"Running ad hoc code signing\")
        set(SIGN_FLAGS --force --sign -)
        set(DEEP_SIGN_FLAGS --force --deep --sign -)
    endif()
    
    # Sign the main executable first
    execute_process(
        COMMAND codesign \${DEEP_SIGN_FLAGS} ${CONTENTS_DIR}/MacOS/CrystalExplorer
        RESULT_VARIABLE SIGN_RESULT
        ERROR_VARIABLE SIGN_ERROR
    )
    if(NOT SIGN_RESULT EQUAL 0)
        message(FATAL_ERROR \"Failed to sign main executable: \${SIGN_ERROR}\")
    endif()
    
    # Find and sign all frameworks and dylibs individually
    file(GLOB_RECURSE FRAMEWORKS \"${CONTENTS_DIR}/Frameworks/*.framework\")
    file(GLOB_RECURSE DYLIBS \"${CONTENTS_DIR}/*.dylib\")
    file(GLOB_RECURSE PLUGINS \"${CONTENTS_DIR}/PlugIns/*\")
    
    foreach(ITEM IN LISTS FRAMEWORKS DYLIBS PLUGINS)
        if(EXISTS \"\${ITEM}\")
            execute_process(
                COMMAND codesign \${SIGN_FLAGS} \"\${ITEM}\"
                RESULT_VARIABLE ITEM_SIGN_RESULT
                ERROR_VARIABLE ITEM_SIGN_ERROR
            )
            if(NOT ITEM_SIGN_RESULT EQUAL 0)
                message(WARNING \"Failed to sign \${ITEM}: \${ITEM_SIGN_ERROR}\")
            endif()
        endif()
    endforeach()
    
    # Sign the entire app bundle last
    execute_process(
        COMMAND codesign \${DEEP_SIGN_FLAGS} ${APPS}
        RESULT_VARIABLE SIGN_APP_RESULT
        ERROR_VARIABLE SIGN_APP_ERROR
    )
    if(NOT SIGN_APP_RESULT EQUAL 0)
        message(FATAL_ERROR \"Failed to sign app bundle: \${SIGN_APP_ERROR}\")
    endif()
    
    # Verify the signature
    execute_process(
        COMMAND codesign --verify --verbose=2 ${APPS}
        RESULT_VARIABLE VERIFY_RESULT
        ERROR_VARIABLE VERIFY_ERROR
    )
    if(NOT VERIFY_RESULT EQUAL 0)
        message(WARNING \"Code signature verification failed: \${VERIFY_ERROR}\")
    else()
        message(STATUS \"Code signing completed successfully\")
    endif()
" COMPONENT Runtime)
