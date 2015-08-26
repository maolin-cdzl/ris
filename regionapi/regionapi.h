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

REGIONAPI_EXPORT int region_stop();


REGIONAPI_EXPORT void* region_open();

REGIONAPI_EXPORT void region_close(void* s);

REGIONAPI_EXPORT int region_new_payload(void* s,const char* uuid);

REGIONAPI_EXPORT int region_rm_payload(void* s,const char* uuid);

REGIONAPI_EXPORT int region_new_service(void* s,const char* uuid,const char* address);

REGIONAPI_EXPORT int region_rm_service(void* s,const char* uuid);

#ifdef __cplusplus
}
#endif
