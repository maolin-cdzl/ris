#include <glog/logging.h>
#include "tracker/subscriber.h"
#include "ris/regionpub.pb.h"
#include "zmqx/zprotobuf++.h"


RISubscriber::RISubscriber(zloop_t* loop) :
	m_loop(loop),
	m_disp(new ZDispatcher(loop))
{

}

RISubscriber::~RISubscriber() {
	stop();
}

std::shared_ptr<Dispatcher> RISubscriber::make_dispatcher() {
	auto disp = std::make_shared<Dispatcher>();
	disp->set_default(std::bind(&RISubscriber::defaultProcess,this,std::placeholders::_1,std::placeholders::_2));
	disp->register_processer(region::pub::Region::descriptor(),std::bind(&RISubscriber::onRegion,this,std::placeholders::_1));
	disp->register_processer(region::pub::RmRegion::descriptor(),std::bind(&RISubscriber::onRmRegion,this,std::placeholders::_1));
	disp->register_processer(region::pub::Service::descriptor(),std::bind(&RISubscriber::onService,this,std::placeholders::_1));
	disp->register_processer(region::pub::RmService::descriptor(),std::bind(&RISubscriber::onRmService,this,std::placeholders::_1));
	disp->register_processer(region::pub::Payload::descriptor(),std::bind(&RISubscriber::onPayload,this,std::placeholders::_1));
	disp->register_processer(region::pub::RmPayload::descriptor(),std::bind(&RISubscriber::onRmPayload,this,std::placeholders::_1));

	return disp;
}


int RISubscriber::start(const std::string& address,const std::shared_ptr<IRIObserver>& ob) {
	int result = -1;
	assert( ! address.empty() );
	assert( ob );

	zsock_t* sub = nullptr;
	do {
		sub = zsock_new_sub(address.c_str(),"");
		if( nullptr == sub ) {
			LOG(FATAL) << "Subscriber can NOT connect to: " << address;
			break;
		} else {
			DLOG(INFO) << "Subscriber connect to: " << address;
		}

		if( -1 == m_disp->start(&sub,make_dispatcher()) ) {
			LOG(FATAL) << "RISubscriber start dispatcher loop failed";
			break;
		}
		m_observer = ob;
		result = 0;
	} while( 0 );

	if( sub ) {
		zsock_destroy(&sub);
	}
	if( -1 == result ) {
		if( m_observer ) {
			m_observer.reset();
		}
	}
	return result;
}

void RISubscriber::stop() {
	m_disp->stop();
	m_observer.reset();
}

int RISubscriber::setObserver(const std::shared_ptr<IRIObserver>& ob) {
	assert( ob );
	if( m_disp->isActive() ) {
		m_observer = ob;
		return 0;
	} else {
		return -1;
	}
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
	if( p->has_bus_address() ) {
		region.bus_address = p->bus_address();
	}
	if( p->has_snapshot_address() ) {
		region.snapshot_address = p->snapshot_address();
	}
	region.timeval = ri_time_now();

	DLOG(INFO) << "RISubscriber recv region: " << region.id << "(" << region.version << ")";
	m_observer->onRegion(region);
}

void RISubscriber::onRmRegion(const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<region::pub::RmRegion>(msg);
	assert(p);

	DLOG(INFO) << "RISubscriber recv rm region: " << p->uuid();
	m_observer->onRmRegion(p->uuid());
}

void RISubscriber::onService(const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<region::pub::Service>(msg);
	assert(p);

	Service svc;
	svc.name = p->name();
	svc.address = p->address();
	svc.timeval = ri_time_now();

	DLOG(INFO) << "RISubscriber recv service: " << svc.name << " in region:" << p->region().uuid() << "(" << p->region().version() << ")";
	m_observer->onService(p->region().uuid(),p->region().version(),svc);
}

void RISubscriber::onRmService(const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<region::pub::RmService>(msg);
	assert(p);

	DLOG(INFO) << "RISubscriber recv rm service: " << p->name() << " in region:" << p->region().uuid() << "(" << p->region().version() << ")";
	m_observer->onRmService(p->region().uuid(),p->region().version(),p->name());
}

void RISubscriber::onPayload(const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<region::pub::Payload>(msg);
	assert(p);

	Payload pl;
	pl.id = p->uuid();
	pl.timeval = ri_time_now();

	DLOG(INFO) << "RISubscriber recv payload: " << p->uuid() << " in region:" << p->region().uuid() << "(" << p->region().version() << ")";
	m_observer->onPayload(p->region().uuid(),p->region().version(),pl);
}

void RISubscriber::onRmPayload(const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<region::pub::RmPayload>(msg);
	assert(p);

	DLOG(INFO) << "RISubscriber recv rm payload: " << p->uuid() << " in region:" << p->region().uuid() << "(" << p->region().version() << ")";
	m_observer->onRmPayload(p->region().uuid(),p->region().version(),p->uuid());
}


