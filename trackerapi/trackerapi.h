#pragma once

#include <czmq.h>

#ifdef _MSC_VER
#	ifdef TRACKERAPI_BUILDING
#		define TRACKERAPI_EXPORT	__declspec(dllexport)
#	else
#		define TRACKERAPI_EXPORT	__declspec(dllimport)
#	endif
#else
#	ifdef TRACKERAPI_BUILDING
#		define TRACKERAPI_EXPORT	__attribute__((__visibility__("default")))
#	else
#		define TRACKERAPI_EXPORT
#	endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

TRACKERAPI_EXPORT int tracker_start(const char* confile,int standalone);

TRACKERAPI_EXPORT int tracker_stop();

#ifdef __cplusplus
}
#endif
