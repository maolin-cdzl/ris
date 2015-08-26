#include "regionapi.h"
#include "ris/region/regionactor.h"
#include <czmq.h>
#include <glog/logging.h>

int g_standalone = 0;
static RIRegionActor* g_actor = nullptr;

extern "C" REGIONAPI_EXPORT int region_start(const char* confile,int standalone) {
	if( confile == nullptr )
		return -1;

	g_standalone = standalone;
	if( g_standalone ) {
		google::InitGoogleLogging("region");
		FLAGS_log_dir = "./log";
		zsys_init();
	}

	g_actor = new RIRegionActor();
	if( -1 == g_actor->start(confile) ) {
		delete g_actor;
		g_actor = nullptr;
		return -1;
	} else {
		return 0;
	}
}


extern "C" REGIONAPI_EXPORT int region_stop() {
	if( g_actor ) {
		g_actor->stop();
		delete g_actor;
		g_actor = nullptr;
	}

	if( g_standalone ) {
		zsys_shutdown();
	}
	return 0;
}

extern "C" {

struct region_api_t {
	zsock_t*			req;
};

}

extern "C" REGIONAPI_EXPORT void* region_open() {
	zsock_t* req = nullptr;

	do {
		if( nullptr == g_actor )
			break;
		req = zsock_new(ZMQ_REQ);
		if( nullptr == req )
			break;
		if( -1 == zsock_connect(req,"%s",g_actor->address().c_str()) ) {
			zsock_destroy(&req);
			break;
		}
	} while( 0 );

	return req;
}

extern "C" REGIONAPI_EXPORT void region_close(void* s) {
	zsock_t* req = (zsock_t*) s;

	if( req ) {
		zsock_destroy(&req);
	}
}

extern "C" REGIONAPI_EXPORT int region_new_payload(void* s,const char* uuid) {
	assert(s);
	assert(uuid);

	int result = -1;
	zsock_t* req = (zsock_t*) s;
	char* str = nullptr;
	do {
		zstr_sendm(req,"#pld");
		zstr_send(req,uuid);

		str = zstr_recv(req);
		if( 0 == strcmp(str,"ok") )
			result = 0;
	} while(0);

	if( str ) {
		free(str);
	}

	return result;
}

extern "C" REGIONAPI_EXPORT int region_rm_payload(void* s,const char* uuid) {
	assert(s);
	assert(uuid);

	int result = -1;
	zsock_t* req = (zsock_t*) s;
	char* str = nullptr;
	do {
		zstr_sendm(req,"#delpld");
		zstr_send(req,uuid);

		str = zstr_recv(req);
		if( 0 == strcmp(str,"ok") )
			result = 0;
	} while(0);

	if( str ) {
		free(str);
	}

	return result;
}

extern "C" REGIONAPI_EXPORT int region_new_service(void* s,const char* uuid,const char* address) {
	assert(s);
	assert(uuid);
	assert(address);

	int result = -1;
	zsock_t* req = (zsock_t*) s;
	char* str = nullptr;
	do {
		zstr_sendm(req,"#svc");
		zstr_sendm(req,uuid);
		zstr_send(req,address);

		str = zstr_recv(req);
		if( 0 == strcmp(str,"ok") )
			result = 0;
	} while(0);

	if( str ) {
		free(str);
	}

	return result;
}

extern "C" REGIONAPI_EXPORT int region_rm_service(void* s,const char* uuid) {
	assert(s);
	assert(uuid);

	int result = -1;
	zsock_t* req = (zsock_t*) s;
	char* str = nullptr;
	do {
		zstr_sendm(req,"#delsvc");
		zstr_send(req,uuid);

		str = zstr_recv(req);
		if( 0 == strcmp(str,"ok") )
			result = 0;
	} while(0);

	if( str ) {
		free(str);
	}

	return result;
}

