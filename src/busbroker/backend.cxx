#include <unordered_set>
#include <glog/logging.h>
#include "busbroker/backend.h"

BusBackend::BusBackend(const std::shared_ptr<BusBrokerCtx>& ctx) :
	m_ctx(ctx)
{
}

BusBackend::~BusBackend() {
	stop();
}

int BusBackend::start() {
	return m_runner.start(std::bind(&BusBackend::run,this,std::placeholders::_1));
}

void BusBackend::stop() {
	m_runner.stop();
}

void BusBackend::run(zsock_t* pipe) {
	zsock_t* backend = nullptr;
	zsock_t* backend_worker = nullptr;
	zsock_t* frontend_reply = nullptr;
	zpoller_t* poller = nullptr;

	backend = zsock_new(ZMQ_ROUTER);
	CHECK_NOTNULL(backend);
	CHECK_NE(-1,zsock_bind(backend,"%s",m_ctx->backend_address.c_str())) << "backend can not bind to: " << m_ctx->backend_address;

	backend_worker = zsock_new(ZMQ_PULL);
	CHECK_NOTNULL(backend_worker);
	CHECK_NE(-1,zsock_bind(backend_worker,"%s",m_ctx->backend_worker_address.c_str())) << "backend worker can not bind to: " << m_ctx->backend_worker_address;

	frontend_reply = zsock_new(ZMQ_PUSH);
	CHECK_NOTNULL(frontend_reply);
	CHECK_NE(-1,zsock_connect(frontend_reply,"%s",m_ctx->frontend_reply_address.c_str())) << "backend can not connect to: " << m_ctx->frontend_reply_address;

	poller = zpoller_new(pipe,backend,backend_worker);
	CHECK_NOTNULL(poller);

	std::unordered_set<std::string> regions;
	zsock_signal(pipe,0);

	while( ! zsys_interrupted ) {
		void* reader = zpoller_wait(poller,-1);

		if( reader == pipe ) {
			// $TERM signal
			zmsg_t* msg = zmsg_recv(reader);
			zmsg_destroy(&msg);
			break;
		} else if( reader == backend ) {
			zmsg_t* msg = zmsg_recv(reader);
			if( msg ) {
				// because backend is ROUTER,remove source address first
				zframe_t* addr = zmsg_pop(msg);
				if( addr ) {
					zframe_destroy(&addr);
					if( zmsg_size(msg) ) {
						zmsg_send(&msg,frontend_reply);
					}
				}
			}
			if( msg ) {
				zmsg_destroy(&msg);
			}
		} else if( reader == backend_worker ) {
			zmsg_t* msg = zmsg_recv(reader);
			if( msg ) {
				char* addr = zmsg_popstr(msg);
				if( addr ) {
					if( regions.find(addr) == regions.end() ) {
						if( -1 != zsock_connect(backend,"%s",addr) ) {
						}
					}
					zstr_free(&addr);
				}
			}
		} else {
			CHECK(false) << "Unknown reader";
		}
	}
}

