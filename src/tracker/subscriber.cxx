#include <glog/logging.h>
#include "ris/tracker/subscriber.h"
#include "ris/regionpub.pb.h"
#include "zmqx/zprotobuf++.h"


RISubscriber::RISubscriber(zloop_t* loop) :
	m_loop(loop),
	m_sub(nullptr),
	m_disp(new ZDispatcher(loop))
{

}

RISubscriber::~RISubscriber() {
	stop();
}

int RISubscriber::startLoop() {
	assert(m_sub);

	auto disp = std::make_shared<Dispatcher>();
	disp->set_member_default(&RISubscriber::defaultProcess,this);
	disp->register_member_processer(region::pub::Region::descriptor(),&RISubscriber::onRegion,this);
	disp->register_member_processer(region::pub::RmRegion::descriptor(),&RISubscriber::onRmRegion,this);
	disp->register_member_processer(region::pub::Service::descriptor(),&RISubscriber::onService,this);
	disp->register_member_processer(region::pub::RmService::descriptor(),&RISubscriber::onRmService,this);
	disp->register_member_processer(region::pub::Payload::descriptor(),&RISubscriber::onPayload,this);
	disp->register_member_processer(region::pub::RmPayload::descriptor(),&RISubscriber::onRmPayload,this);

	return m_disp->start(m_sub,disp);
}

void RISubscriber::stopLoop() {
	m_disp->stop();
}

int RISubscriber::start(const std::string& address,const std::shared_ptr<IRIObserver>& ob) {
	int result = -1;
	assert( ! address.empty() );
	assert( ob );

	do {
		if( m_sub )
			break;
		m_observer = ob;
		m_sub = zsock_new(ZMQ_SUB);

		if( -1 == zsock_connect(m_sub,"%s",address.c_str()) ) {
			LOG(ERROR) << "Subscriber can NOT connect to: " << address;
			break;
		}

		if( -1 == startLoop() )
			break;
		result = 0;
	} while( 0 );

	if( -1 == result ) {
		if( m_sub ) {
			zsock_destroy(&m_sub);
		}
		if( m_observer ) {
			m_observer.reset();
		}
	}
	return result;
}

int RISubscriber::stop() {
	if( m_sub == nullptr )
		return -1;

	zloop_reader_end(m_loop,m_sub);
	zsock_destroy(&m_sub);
	return 0;
}


void RISubscriber::defaultProcess(const std::shared_ptr<google::protobuf::Message>& msg,int) {
	if( msg ) {
		LOG(WARNING) << "RISubscriber recv unexpected message: " << msg->GetTypeName();
	} else {
		LOG(WARNING) << "RISubscribe recv no protobuf message";
	}
}

void RISubscriber::onRegion(const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<region::pub::Region>(msg);
	assert(p);
	Region region;
	region.id = p->region().uuid();
	region.version = p->region().version();
	if( p->has_idc() ) {
		region.idc = p->idc();
	}
	if( p->has_msg_address() ) {
		region.msg_address = p->msg_address();
	}
	if( p->has_snapshot_address() ) {
		region.snapshot_address = p->snapshot_address();
	}
	region.timeval = ri_time_now();
	m_observer->onRegion(region);
}

void RISubscriber::onRmRegion(const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<region::pub::RmRegion>(msg);
	assert(p);

	m_observer->onRmRegion(p->uuid());
}

void RISubscriber::onService(const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<region::pub::Service>(msg);
	assert(p);

	Service svc;
	svc.name = p->name();
	svc.address = p->address();
	svc.timeval = ri_time_now();
	m_observer->onService(p->region().uuid(),p->region().version(),svc);
}

void RISubscriber::onRmService(const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<region::pub::RmService>(msg);
	assert(p);
	m_observer->onRmService(p->region().uuid(),p->region().version(),p->name());
}

void RISubscriber::onPayload(const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<region::pub::Payload>(msg);
	assert(p);

	Payload pl;
	pl.id = p->uuid();
	pl.timeval = ri_time_now();

	m_observer->onPayload(p->region().uuid(),p->region().version(),pl);
}

void RISubscriber::onRmPayload(const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<region::pub::RmPayload>(msg);
	assert(p);
	m_observer->onRmPayload(p->region().uuid(),p->region().version(),p->uuid());
}


