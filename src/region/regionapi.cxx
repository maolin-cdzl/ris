#include <czmq.h>
#include <glog/logging.h>

#include "region/regionapi.h"
#include "region/regionactor.h"
#include "region/regionctx.h"
#include "ris/regionapi.pb.h"
#include "zmqx/zprotobuf++.h"

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

	auto ctx = loadRegionCtx(confile);
	if( nullptr == ctx )
		return -1;

	g_actor = new RIRegionActor();
	if( -1 == g_actor->start(ctx) ) {
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

extern "C" REGIONAPI_EXPORT int region_wait() {
	if( g_actor ) {
		return g_actor->wait();
	} else {
		return -1;
	}
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
		auto ctx = g_actor->getCtx();
		if( nullptr == ctx )
			break;
		req = zsock_new(ZMQ_REQ);
		if( nullptr == req )
			break;
		if( -1 == zsock_connect(req,"%s",ctx->api_address.c_str()) ) {
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

	region::api::AddPayload msg;
	msg.set_uuid(uuid);
	if( 0 == zpb_send(s,msg) ) {
		region::api::Result result;
		if( 0 == zpb_recv(result,s) ) {
			return result.result();
		}
	}
	return -1;
}

extern "C" REGIONAPI_EXPORT int region_rm_payload(void* s,const char* uuid) {
	assert(s);
	assert(uuid);

	region::api::RmPayload msg;
	msg.set_uuid(uuid);
	if( 0 == zpb_send(s,msg) ) {
		region::api::Result result;
		if( 0 == zpb_recv(result,s) ) {
			return result.result();
		}
	}
	return -1;
}

extern "C" REGIONAPI_EXPORT int region_new_service(void* s,const char* name,const char* address) {
	assert(s);
	assert(name);
	assert(address);

	region::api::AddService msg;
	msg.set_name(name);
	msg.set_address(address);
	if( 0 == zpb_send(s,msg) ) {
		region::api::Result result;
		if( 0 == zpb_recv(result,s) ) {
			return result.result();
		}
	}
	return -1;
}

extern "C" REGIONAPI_EXPORT int region_rm_service(void* s,const char* name) {
	assert(s);
	assert(name);

	region::api::RmService msg;
	msg.set_name(name);
	if( 0 == zpb_send(s,msg) ) {
		region::api::Result result;
		if( 0 == zpb_recv(result,s) ) {
			return result.result();
		}
	}
	return -1;
}

