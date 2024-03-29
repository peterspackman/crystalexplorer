#pragma once
#define XSTRINGIFYX(a) STRINGIFYX(a)
#define STRINGIFYX(a) #a
inline const char *CX_VERSION_MAJOR = XSTRINGIFYX(PROJECT_VERSION_MAJOR);
inline const char *CX_VERSION_MINOR = XSTRINGIFYX(PROJECT_VERSION_MINOR);
inline const char *CX_GIT_REVISION = XSTRINGIFYX(PROJECT_GIT_REVISION);
inline const char *CX_VERSION = XSTRINGIFYX(PROJECT_VERSION);
inline const char *CX_BUILD_DATE = XSTRINGIFYX(PROJECT_BUILD_DATE);
inline const char *HSPrevVersion = XSTRINGIFYX(HS_PREVIOUS_VERSION);
#undef STRINGIFYX
#undef XSTRINGIFYX
