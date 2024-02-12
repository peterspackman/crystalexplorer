if(EXISTS ${PROJECT_SOURCE_DIR}/.git)
    find_package(Git)
endif()

function(set_git_revision PROJECT_GIT_REVISION)
    if(${GIT_FOUND})
        execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            OUTPUT_VARIABLE REVISION_STRING
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
        set(REVISION_STRING "Unknown git revision")
    endif()
    set(${PROJECT_GIT_REVISION} ${REVISION_STRING} PARENT_SCOPE)
endfunction(set_git_revision)
