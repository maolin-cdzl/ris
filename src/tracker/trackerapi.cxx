#include <czmq.h>
#include <glog/logging.h>

#include "tracker/trackerapi.h"
#include "tracker/trackeractor.h"

static RITrackerActor* g_actor = nullptr;

extern "C" TRACKERAPI_EXPORT int tracker_start(const char* confile) {
	if( confile == nullptr )
		return -1;

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

	return 0;
}


extern "C" TRACKERAPI_EXPORT int tracker_wait() {
	if( g_actor ) {
		return g_actor->wait();
	} else {
		return -1;
	}
}

