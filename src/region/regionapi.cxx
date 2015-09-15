#include <czmq.h>
#include <glog/logging.h>

#include "region/regionapi.h"
#include "region/regionactor.h"
#include "region/regionctx.h"
#include "ris/regionapi.pb.h"
#include "zmqx/zhelper.h"
#include "zmqx/zprotobuf++.h"


extern "C" REGIONAPI_EXPORT void* region_new_str(const char* confstr,int initenv) {
	RIRegionActor* region = nullptr;

	do {
		if( confstr == nullptr )
			break;

		if( initenv ) {
			google::InitGoogleLogging("region");
			FLAGS_log_dir = "./log";
			zsys_init();
		}

		auto ctx = RegionCtx::loadStr(confstr);
		if( nullptr == ctx )
			break;

		region = new RIRegionActor();
		if( -1 == region->start(ctx) ) {
			break;
		}

		return region;
	} while(0);

	if( region ) {
		delete region;
	}
	return nullptr;
}

extern "C" REGIONAPI_EXPORT void* region_new(const char* confile,int initenv) {
	RIRegionActor* region = nullptr;

	do {
		if( confile == nullptr )
			break;

		if( initenv ) {
			google::InitGoogleLogging("region");
			FLAGS_log_dir = "./log";
			zsys_init();
		}

		auto ctx = RegionCtx::loadFile(confile);
		if( nullptr == ctx )
			break;

		region = new RIRegionActor();
		if( -1 == region->start(ctx) ) {
			break;
		}

		return region;
	} while(0);

	if( region ) {
		delete region;
	}
	return nullptr;
}


extern "C" REGIONAPI_EXPORT void region_destroy(void* p) {
	RIRegionActor* region = (RIRegionActor*)p;
	if( region ) {
		region->stop();
		delete region;
	}
}

extern "C" REGIONAPI_EXPORT int region_wait(void* p) {
	RIRegionActor* region = (RIRegionActor*)p;
	if( region ) {
		int state = region->wait();
		delete region;
		return state;
	} else {
		return -1;
	}
}

