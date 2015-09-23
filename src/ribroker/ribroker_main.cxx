#include <unistd.h>
#include <string>
#include <iostream>
#include <zmq.h>
#include <gflags/gflags.h>
#include <glog/logging.h>


DEFINE_bool(daemon,false,"run as daemon");
DEFINE_string(frontend,"","frontend address,talk to publishers");
DEFINE_string(backend,"","backend address,talk to subscribers");

void ribroker_start(void* ctx,const char* front_addr,const char* back_addr);

int main(int argc,char* argv[]) {
	google::ParseCommandLineFlags(&argc,&argv,false);
	google::InitGoogleLogging(argv[0]);
	if( FLAGS_frontend.empty() || FLAGS_backend.empty() ) {
		std::cerr << "Must define listen address for both frontend and backend!" << std::endl;
		return -1;
	}

	if( FLAGS_daemon ) {
		daemon(1,0);
	}

	void* ctx = zmq_ctx_new();
	CHECK_NOTNULL(ctx);
	
	ribroker_start(ctx,FLAGS_frontend.c_str(),FLAGS_backend.c_str());

	if( ctx ) {
		zmq_ctx_destroy(ctx);
	}
	return 0;
}




