#include <czmq.h>
#include <glog/logging.h>

#include "trackerapi.h"
#include "tracker/trackeractor.h"

int g_standalone = 0;
static RITrackerActor* g_actor = nullptr;

extern "C" TRACKERAPI_EXPORT int tracker_start(const char* confile,int standalone) {
	if( confile == nullptr )
		return -1;

	g_standalone = standalone;
	if( g_standalone ) {
		google::InitGoogleLogging("tracker");
		FLAGS_log_dir = "./log";
		zsys_init();
	}

	auto ctx = loadTrackerCtx(confile);
	if( nullptr == ctx )
		return -1;

	g_actor = new RITrackerActor();
	if( -1 == g_actor->start(ctx) ) {
		delete g_actor;
		g_actor = nullptr;
		return -1;
	} else {
		return 0;
	}
}

extern "C" TRACKERAPI_EXPORT int tracker_stop() {
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


extern "C" TRACKERAPI_EXPORT int tracker_wait() {
	if( g_actor ) {
		return g_actor->wait();
	} else {
		return -1;
	}
}

