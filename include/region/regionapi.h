#pragma once

#include <czmq.h>

#ifdef _MSC_VER
#	ifdef REGIONAPI_BUILDING
#		define REGIONAPI_EXPORT	__declspec(dllexport)
#	else
#		define REGIONAPI_EXPORT	__declspec(dllimport)
#	endif
#else
#	ifdef REGIONAPI_BUILDING
#		define REGIONAPI_EXPORT	__attribute__((__visibility__("default")))
#	else
#		define REGIONAPI_EXPORT
#	endif
#endif

#ifdef __cplusplus
extern "C" {
#endif


REGIONAPI_EXPORT int region_start(const char* confile,int standalone);

REGIONAPI_EXPORT int region_start_str(const char* confstr,int standalone);

REGIONAPI_EXPORT int region_stop();

// wait tracker shutdown,it will block caller
REGIONAPI_EXPORT int region_wait();

#ifdef __cplusplus
}
#endif
