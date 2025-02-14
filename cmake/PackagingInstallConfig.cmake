if(APPLE)
include(PackageMacOS)
elseif(WIN32)
include(PackageWindows)
elseif(UNIX)
include(PackageLinux)
endif(APPLE)

include(CPack)
