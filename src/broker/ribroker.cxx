#include <unistd.h>
#include <string>
#include <iostream>
#include <zmq.h>
#include <gflags/gflags.h>


DEFINE_bool(daemon,false,"run as daemon");
DEFINE_string(frontend,"","frontend address,talk to publishers");
DEFINE_string(backend,"","backend address,talk to subscribers");

int main(int argc,char* argv[]) {
	google::ParseCommandLineFlags(&argc,&argv,false);
	if( FLAGS_frontend.empty() || FLAGS_backend.empty() ) {
		std::cerr << "Must define listen address for both frontend and backend!" << std::endl;
		return -1;
	}

	if( FLAGS_daemon ) {
		daemon(1,0);
	}

	void* ctx = nullptr;
	void* frontend = nullptr;
	void* backend = nullptr;

	do {
		ctx = zmq_ctx_new();
		if( nullptr == ctx )
			break;
		frontend = zmq_socket(ctx,ZMQ_SUB);
		if( nullptr == frontend )
			break;
		if( -1 == zmq_bind(frontend,FLAGS_frontend.c_str()) )
			break;
		backend = zmq_socket(ctx,ZMQ_PUB);
		if( nullptr == backend )
			break;
		if( -1 == zmq_bind(backend,FLAGS_backend.c_str()) )
			break;

		zmq_device(ZMQ_FORWARDER,frontend,backend);
	} while( 0 );
	
	if( frontend ) {
		zmq_close(frontend);
	}
	if( backend ) {
		zmq_close(backend);
	}
	if( ctx ) {
		zmq_ctx_destroy(ctx);
	}
	return 0;
}



