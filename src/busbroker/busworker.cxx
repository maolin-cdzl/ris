#include <memory>
#include <glog/logging.h>
#include "busbroker/busworker.h"
#include "trackercli/trackersession.h"
#include "zmqx/zenvelope.h"
#include "zmqx/zprotobuf++.h"
#include "ris/bus.pb.h"

static void do_work(zsock_t* frontend,zsock_t* backend,const std::shared_ptr<TrackerSession>& tracker);

BusWorker::BusWorker() {
}

BusWorker::~BusWorker() {
}

int BusWorker::start(const std::string& tracker,const std::string& frontend,const std::string& backend) {
	CHECK(! frontend.empty());
	CHECK(! backend.empty());

	return m_runner.start(std::bind(&BusWorker::run,this,tracker,frontend,backend,std::placeholders::_1));
}

void BusWorker::stop() {
	m_runner.stop();
}

void BusWorker::run(const std::string& tk_addr,const std::string& ft_addr,const std::string& be_addr,zsock_t* pipe) {
	int result = 0;
	auto tracker = std::make_shared<TrackerSession>();
	result = tracker->connect(tk_addr);
	CHECK_EQ(0,result);

	zsock_t* frontend = nullptr;
	zsock_t* backend = nullptr;

	backend = zsock_new(ZMQ_PUSH);
	CHECK_NOTNULL(backend);
	result = zsock_connect(backend,"%s",be_addr.c_str());
	CHECK_EQ(0,result);

	frontend = zsock_new(ZMQ_PULL);
	CHECK_NOTNULL(frontend);
	result = zsock_connect(frontend,"%s",ft_addr.c_str());
	CHECK_EQ(0,result);
		
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
			do_work(frontend,backend,tracker);		
		}
	}

	zpoller_destroy(&poller);
	zsock_destroy(&frontend);
	zsock_destroy(&backend);
}

static void do_work(zsock_t* frontend,zsock_t* backend,const std::shared_ptr<TrackerSession>& tracker) {
	do {
		auto envelope = ZEnvelope::recv(frontend);
		if( ! envelope )
			break;

		bus::SendHeader header;
		if( -1 == zpb_recv(header,frontend) )
			break;

		std::list<std::string> payloads;
		std::copy(header.targets().begin(),header.targets().end(),std::back_inserter(payloads));
	} while(0);

	zsock_flush(frontend);
}


