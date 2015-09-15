#include <czmq.h>
#include <glog/logging.h>

#include "tracker/trackerapi.h"
#include "tracker/trackeractor.h"


extern "C" TRACKERAPI_EXPORT void* tracker_new(const char* confile) {
	RITrackerActor* tracker = nullptr;
	do {
		if( confile == nullptr )
			break;

		auto ctx = TrackerCtx::loadFile(confile);
		if( nullptr == ctx )
			break;

		tracker = new RITrackerActor();
		if( -1 == tracker->start(ctx) ) {
			break;
		}
		return tracker;
	} while(0);

	if( tracker ) {
		delete tracker;
	}
	return nullptr;
}

extern "C" TRACKERAPI_EXPORT void* tracker_new_str(const char* confstr) {
	RITrackerActor* tracker = nullptr;
	do {
		if( confstr == nullptr )
			break;

		auto ctx = TrackerCtx::loadStr(confstr);
		if( nullptr == ctx )
			break;

		tracker = new RITrackerActor();
		if( -1 == tracker->start(ctx) ) {
			break;
		}
		return tracker;
	} while(0);

	if( tracker ) {
		delete tracker;
	}
	return nullptr;
}

extern "C" TRACKERAPI_EXPORT void tracker_destroy(void* p) {
	RITrackerActor* tracker = (RITrackerActor*)p;
	if( tracker ) {
		tracker->stop();
		delete tracker;
	}
}


extern "C" TRACKERAPI_EXPORT int tracker_wait(void* p) {
	RITrackerActor* tracker = (RITrackerActor*)p;
	if( tracker ) {
		int state = tracker->wait();
		delete tracker;
		return state;
	} else {
		return -1;
	}
}

