#include <glog/logging.h>
#include "tracker/subscriber.h"
#include "ris/pub.pb.h"
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
	disp->set_default(std::bind<int>(&RISubscriber::defaultProcess,this,std::placeholders::_1));
	disp->register_processer(pub::Region::descriptor(),std::bind<int>(&RISubscriber::onRegion,this,std::placeholders::_1));
	disp->register_processer(pub::RmRegion::descriptor(),std::bind<int>(&RISubscriber::onRmRegion,this,std::placeholders::_1));
	disp->register_processer(pub::Service::descriptor(),std::bind<int>(&RISubscriber::onService,this,std::placeholders::_1));
	disp->register_processer(pub::RmService::descriptor(),std::bind<int>(&RISubscriber::onRmService,this,std::placeholders::_1));
	disp->register_processer(pub::Payload::descriptor(),std::bind<int>(&RISubscriber::onPayload,this,std::placeholders::_1));
	disp->register_processer(pub::RmPayload::descriptor(),std::bind<int>(&RISubscriber::onRmPayload,this,std::placeholders::_1));

	return disp;
}


int RISubscriber::start(const std::string& address,const std::shared_ptr<IRIObserver>& ob) {
	int result = -1;
	CHECK( ! address.empty() );
	CHECK( ob );

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
	CHECK( ob );
	if( m_disp->isActive() ) {
		m_observer = ob;
		return 0;
	} else {
		return -1;
	}
}

int RISubscriber::defaultProcess(const std::shared_ptr<google::protobuf::Message>& msg) {
	LOG(WARNING) << "RISubscriber recv unexpected message: " << msg->GetTypeName();
	return 0;
}

int RISubscriber::onRegion(const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<pub::Region>(msg);
	CHECK(p);

	Region region;
	region.id = p->region().uuid();
	region.version = p->region().version();
	region.idc = p->idc();
	region.bus_address = p->bus_address();
	region.snapshot_address = p->snapshot_address();
	region.timeval = ri_time_now();

	DLOG(INFO) << "RISubscriber recv region: " << region.id << "(" << region.version << ")";
	m_observer->onRegion(region);
	return 0;
}

int RISubscriber::onRmRegion(const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<pub::RmRegion>(msg);
	CHECK(p);

	DLOG(INFO) << "RISubscriber recv rm region: " << p->uuid();
	m_observer->onRmRegion(p->uuid());
	return 0;
}

int RISubscriber::onService(const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<pub::Service>(msg);
	CHECK(p);

	Service svc;
	svc.name = p->name();
	svc.address = p->address();
	svc.timeval = ri_time_now();

	DLOG(INFO) << "RISubscriber recv service: " << svc.name << " in region:" << p->region().uuid() << "(" << p->region().version() << ")";
	m_observer->onService(p->region().uuid(),p->region().version(),svc);
	return 0;
}

int RISubscriber::onRmService(const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<pub::RmService>(msg);
	CHECK(p);

	DLOG(INFO) << "RISubscriber recv rm service: " << p->name() << " in region:" << p->region().uuid() << "(" << p->region().version() << ")";
	m_observer->onRmService(p->region().uuid(),p->region().version(),p->name());
	return 0;
}

int RISubscriber::onPayload(const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<pub::Payload>(msg);
	CHECK(p);

	Payload pl;
	pl.id = p->uuid();
	pl.timeval = ri_time_now();

	DLOG(INFO) << "RISubscriber recv payload: " << p->uuid() << " in region:" << p->region().uuid() << "(" << p->region().version() << ")";
	m_observer->onPayload(p->region().uuid(),p->region().version(),pl);
	return 0;
}

int RISubscriber::onRmPayload(const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<pub::RmPayload>(msg);
	CHECK(p);

	DLOG(INFO) << "RISubscriber recv rm payload: " << p->uuid() << " in region:" << p->region().uuid() << "(" << p->region().version() << ")";
	m_observer->onRmPayload(p->region().uuid(),p->region().version(),p->uuid());
	return 0;
}


