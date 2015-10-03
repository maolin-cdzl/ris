#include <list>
#include <glog/logging.h>
#include "busbroker/busbroker.h"
#include "busbroker/busworker.h"


BusBroker::BusBroker(const std::shared_ptr<BusBrokerCtx>& ctx) : 
	m_ctx(ctx)
{
}

BusBroker::~BusBroker() {
}

int BusBroker::run() {
	zsock_t* frontend = nullptr;
	zsock_t* frontend_worker = nullptr;
	zsock_t* frontend_reply = nullptr;
	zsock_t* backend = nullptr;
	zsock_t* backend_worker = nullptr;

	zpoller_t* poller = nullptr;

	frontend = zsock_new(ZMQ_ROUTER);
	CHECK_NOTNULL(frontend);
	CHECK_NE(-1,zsock_bind(frontend,"%s",m_ctx->frontend_address.c_str()));

	frontend_worker = zsock_new(ZMQ_PUSH);
	CHECK_NOTNULL(frontend_worker);
	CHECK_NE(-1,zsock_bind(frontend_worker,"%s",m_ctx->frontend_worker_address.c_str()));

	frontend_reply = zsock_new(ZMQ_PULL);
	CHECK_NOTNULL(frontend_reply);
	CHECK_NE(-1,zsock_bind(frontend_reply,"%s",m_ctx->frontend_reply_address.c_str()));

	backend = zsock_new(ZMQ_ROUTER);
	CHECK_NOTNULL(backend);
	CHECK_NE(-1,zsock_bind(backend,"%s",m_ctx->backend_address.c_str()));

	backend_worker = zsock_new(ZMQ_PULL);
	CHECK_NOTNULL(backend_worker);
	CHECK_NE(-1,zsock_bind(backend_worker,"%s",m_ctx->backend_worker_address.c_str()));


	std::list<std::shared_ptr<BusWorker>>	workers;
	for(size_t i=0; i < m_ctx->worker_count; ++i) {
		auto worker = std::make_shared<BusWorker>(m_ctx);
		CHECK_NE(-1,worker->start());
		workers.push_back(worker);
	}

	poller = zpoller_new(frontend_reply,backend,backend_worker,frontend);
	CHECK_NOTNULL(poller);

	while( ! zsys_interrupted ) {
		void* reader = zpoller_wait(poller,-1);
		if( nullptr == reader ) {
			// interrupted
			break;
		}

		if( reader == frontend_reply || reader == backend ) {
			zmsg_t* msg = zmsg_recv(reader);
			if( msg ) {
				zmsg_send(&msg,frontend);
			}
		} else if( reader == backend_worker ) {
		} else if( reader == frontend ) {
			zmsg_t* msg = zmsg_recv(reader);
			if( msg ) {
				zmsg_send(&msg,frontend_worker);
			}
		} else {
			CHECK(false) << "Unknown reader";
		}
	}

	workers.clear();
	zpoller_destroy(&poller);
	zsock_destroy(&frontend);
	zsock_destroy(&frontend_reply);
	zsock_destroy(&frontend_worker);
	zsock_destroy(&backend);
	zsock_destroy(&backend_worker);

	return 0;
}


