#include <czmq.h>
#include <glog/logging.h>

#include "region/regionapi.h"
#include "region/regionactor.h"
#include "region/regionctx.h"
#include "ris/regionapi.pb.h"
#include "zmqx/zhelper.h"
#include "zmqx/zprotobuf++.h"

int g_standalone = 0;
static RIRegionActor* g_actor = nullptr;

extern "C" REGIONAPI_EXPORT int region_start_str(const char* confstr,int standalone) {
	if( confstr == nullptr )
		return -1;

	g_standalone = standalone;
	if( g_standalone ) {
		google::InitGoogleLogging("region");
		FLAGS_log_dir = "./log";
		zsys_init();
	}

	auto ctx = RegionCtx::loadStr(confstr);
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

extern "C" REGIONAPI_EXPORT int region_start(const char* confile,int standalone) {
	if( confile == nullptr )
		return -1;

	g_standalone = standalone;
	if( g_standalone ) {
		google::InitGoogleLogging("region");
		FLAGS_log_dir = "./log";
		zsys_init();
	}

	auto ctx = RegionCtx::loadFile(confile);
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

