add_subdirectory(occ/core)
add_subdirectory(occ/crystal)
add_subdirectory(occ/3rdparty)

set(OCC_INCLUDE_FILES
    ${OCC_CORE_INCLUDE_FILES}
    ${OCC_CRYSTAL_INCLUDE_FILES}
    ${OCC_3RDPARTY_INCLUDE_FILES}
    PARENT_SCOPE
)

set(OCC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
