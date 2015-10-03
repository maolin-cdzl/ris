#include <unordered_map>
#include <memory>
#include <glog/logging.h>
#include "busbroker/busworker.h"
#include "trackercli/trackersession.h"
#include "zmqx/zenvelope.h"
#include "zmqx/zprotobuf++.h"
#include "ris/bus.pb.h"

static void do_work(zsock_t* frontend,zsock_t* backend,zsock_t* reply,const std::shared_ptr<TrackerSession>& tracker);

BusWorker::BusWorker(const std::shared_ptr<BusBrokerCtx>& ctx) :
	m_ctx(ctx)
{
}

BusWorker::~BusWorker() {
	stop();
}

int BusWorker::start() {
	return m_runner.start(std::bind(&BusWorker::run,this,std::placeholders::_1));
}

void BusWorker::stop() {
	m_runner.stop();
}

void BusWorker::run(zsock_t* pipe) {
	auto tracker = std::make_shared<TrackerSession>();
	CHECK_EQ( 0,tracker->connect(m_ctx->tracker_api_address) );

	zsock_t* frontend = nullptr;
	zsock_t* backend = nullptr;
	zsock_t* reply = nullptr;

	backend = zsock_new(ZMQ_PUSH);
	CHECK_NOTNULL(backend);
	CHECK_EQ(0, zsock_connect(backend,"%s",m_ctx->backend_address.c_str()));

	reply = zsock_new(ZMQ_PUSH);
	CHECK_NOTNULL(reply);
	CHECK_EQ(0, zsock_connect(reply,"%s",m_ctx->frontend_reply_address.c_str()));

	frontend = zsock_new(ZMQ_PULL);
	CHECK_NOTNULL(frontend);
	CHECK_EQ(0, zsock_connect(frontend,"%s",m_ctx->frontend_address.c_str()));
		
	zsock_signal(pipe,0);

	zpoller_t* poller = zpoller_new(pipe,frontend);

	while(true) {
		void* reader = zpoller_wait(poller,-1);

		if( nullptr == reader ) {
			break;
		}

		if( pipe == reader ) {
			// $TERM signal
			zmsg_t* msg = zmsg_recv(reader);
			zmsg_destroy(&msg);
			break;
		} else if( frontend == reader ) {
			do_work(frontend,backend,reply,tracker);		
		}
	}

	zpoller_destroy(&poller);
	zsock_destroy(&frontend);
	zsock_destroy(&backend);
}

static void do_work(zsock_t* frontend,zsock_t* backend,zsock_t* reply,const std::shared_ptr<TrackerSession>& tracker) {
	zmsg_t* data = nullptr;
	do {
		auto envelope = ZEnvelope::recv(frontend);
		if( ! envelope )
			break;

		bus::SendMessage header;
		if( -1 == zpb_recv(header,frontend) ) {
			break;
		}
		if( header.msg_id() == 0 || header.payloads_size() == 0 ) {
			break;
		}
		if( !zsock_rcvmore(frontend) ) {
			break;
		}
		data = zmsg_recv(frontend);

		std::list<std::string> payloads;
		std::copy(header.payloads().begin(),header.payloads().end(),std::back_inserter(payloads));

		std::list<RouteInfo> routes = tracker->getPayloadsRouteInfo(payloads);

		// merge same dest route
		std::list<std::string> unfounds;
		std::unordered_map<EndPoint,std::list<std::string>> route_map;
		for(auto it=routes.begin(); it != routes.end(); ++it) {
			if( ! it->endpoint.address.empty() ) {
				route_map[it->endpoint].push_back(it->target);
			} else {
				unfounds.push_back(it->target);
			}
		}

		// sendback failure message
		if( header.failure() && !unfounds.empty() ) {
			bus::Failure failure;
			failure.set_msg_id(header.msg_id());
			std::copy(unfounds.begin(),unfounds.end(),google::protobuf::RepeatedFieldBackInserter(failure.mutable_payloads()));

			envelope->sendm(reply);
			zpb_send(reply,failure);
		}

		// deliver data message
		for(auto it=route_map.begin(); it != route_map.end(); ++it) {
			bus::SendMessage sh;
			sh.set_msg_id(header.msg_id());
			sh.set_failure(header.failure());
			std::copy(it->second.begin(),it->second.end(),google::protobuf::RepeatedPtrFieldBackInserter(sh.mutable_payloads()));

			//TODO
			zstr_sendm(backend,it->first.identity.c_str());	// add region id as address frame
			envelope->sendm(backend);
			zmsg_t* data_dup = zmsg_dup(data);
			zmsg_send(&data_dup,backend);
		}
	} while(0);

	if( data ) {
		zmsg_destroy(&data);
	}
	zsock_flush(frontend);
}


