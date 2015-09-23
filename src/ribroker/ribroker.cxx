#include <zmq.h>
#include <glog/logging.h>


void ribroker_start(void* ctx,const char* front_addr,const char* back_addr) {
	void* frontend = nullptr;
	void* backend = nullptr;

	do {
		frontend = zmq_socket(ctx,ZMQ_SUB);
		CHECK_NOTNULL(frontend );
		if( -1 == zmq_setsockopt(frontend,ZMQ_SUBSCRIBE,"",0) )
			break;
		if( -1 == zmq_bind(frontend,front_addr) )
			break;
		backend = zmq_socket(ctx,ZMQ_PUB);
		CHECK_NOTNULL(backend );
		if( -1 == zmq_bind(backend,back_addr) )
			break;

		zmq_device(ZMQ_FORWARDER,frontend,backend);
	} while( 0 );
	
	if( frontend ) {
		zmq_close(frontend);
	}
	if( backend ) {
		zmq_close(backend);
	}
}



