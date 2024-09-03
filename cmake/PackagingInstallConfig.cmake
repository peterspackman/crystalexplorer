if(APPLE)
    include(PackageMacOS)
elseif(WIN32)
    include(PackageWindows)
endif(APPLE)

include(CPack)
