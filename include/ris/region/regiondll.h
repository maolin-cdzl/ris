#pragma once

#ifdef _MSC_VER
#	ifdef LIBREGION_BUILDING
#		define LIBREGION_EXPORT	__declspec(dllexport)
#	else
#		define LIBREGION_EXPORT	__declspec(dllimport)
#	endif
#else
#	ifdef LIBREGION_BUILDING
#		define LIBREGION_EXPORT	__attribute__((__visibility__("default")))
#	else
#		define LIBREGION_EXPORT
#	endif
#endif


#ifdef __cplusplus
extern "C" {
#endif


LIBREGION_EXPORT int region_start(const char* confile,int standalone);

LIBREGION_EXPORT int region_stop();

#ifdef __cplusplus
}
#endif

