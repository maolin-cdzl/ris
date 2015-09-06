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
	int result = tracker_start(FLAGS_config_file.c_str());
	if( -1 == result )
		return -1;

	tracker_wait();
	zsys_shutdown();
	return 0;
}

