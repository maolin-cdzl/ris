#include "ris/region/regiondll.h"
#include "ris/region/regionactor.h"
#include <czmq.h>
#include <glog/logging.h>

int g_standalone = 0;
static RIRegionActor* g_actor = nullptr;

extern "C" LIBREGION_EXPORT int region_start(const char* confile,int standalone) {
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


extern "C" LIBREGION_EXPORT int region_stop() {
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

