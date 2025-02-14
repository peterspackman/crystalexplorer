# Set DEB package specifics
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Peter Spackman <peterspackman@fastmail.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "science")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

# Set package architecture
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
else()
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
endif()

# Optional: Set package description and other metadata
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CrystalExplorer - A tool for crystal structure visualization")
set(CPACK_PACKAGE_DESCRIPTION "CrystalExplorer is a program for studying molecular crystals and their intermolecular interactions through Hirshfeld surfaces and fingerprint plots.")
