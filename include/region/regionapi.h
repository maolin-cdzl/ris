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


REGIONAPI_EXPORT void* region_new(const char* confile,int initenv);

REGIONAPI_EXPORT void* region_new_str(const char* confstr,int initenv);

REGIONAPI_EXPORT void region_destroy(void* p);

// wait and destroy region,it will block caller
REGIONAPI_EXPORT int region_wait(void* p);

#ifdef __cplusplus
}
#endif
