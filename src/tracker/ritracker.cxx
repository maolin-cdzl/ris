#include <czmq.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include "tracker/trackerapi.h"


DEFINE_bool(daemon,false,"daemon");
DEFINE_string(config_file,"tracker.conf","");

int main(int argc,char* argv[]) {
	google::ParseCommandLineFlags(&argc,&argv,false);
	if( FLAGS_daemon ) {
		daemon(1,0);
	}
	google::InitGoogleLogging(argv[0]);
	zsys_init();
	void* tracker = tracker_new(FLAGS_config_file.c_str());
	CHECK_NOTNULL(tracker);

	tracker_wait(tracker);
	zsys_shutdown();
	return 0;
}

