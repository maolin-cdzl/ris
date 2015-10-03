#include <glog/logging.h>
#include "tracker/subscriber.h"
#include "ris/pub.pb.h"
#include "zmqx/zprotobuf++.h"


RISubscriber::RISubscriber(zloop_t* loop) :
	m_loop(loop),
	m_reader(nullptr)
{

}

RISubscriber::~RISubscriber() {
	stop();
}

std::shared_ptr<sub_dispatcher_t> RISubscriber::make_dispatcher() {
	auto disp = std::make_shared<sub_dispatcher_t>();
	disp->set_default(std::bind<int>(&RISubscriber::defaultProcess,this,std::placeholders::_1,std::placeholders::_2));
	disp->register_processer(pub::PubRegion::descriptor(),std::bind<int>(&RISubscriber::onRegion,this,std::placeholders::_1,std::placeholders::_2));
	disp->register_processer(pub::PubRmRegion::descriptor(),std::bind<int>(&RISubscriber::onRmRegion,this,std::placeholders::_1,std::placeholders::_2));
	disp->register_processer(pub::PubService::descriptor(),std::bind<int>(&RISubscriber::onService,this,std::placeholders::_1,std::placeholders::_2));
	disp->register_processer(pub::PubRmService::descriptor(),std::bind<int>(&RISubscriber::onRmService,this,std::placeholders::_1,std::placeholders::_2));
	disp->register_processer(pub::PubPayload::descriptor(),std::bind<int>(&RISubscriber::onPayload,this,std::placeholders::_1,std::placeholders::_2));
	disp->register_processer(pub::PubRmPayload::descriptor(),std::bind<int>(&RISubscriber::onRmPayload,this,std::placeholders::_1,std::placeholders::_2));

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

		m_reader = make_zpb_reader(m_loop,&sub,make_dispatcher());
		CHECK(m_reader) << "RISubscriber start dispatcher loop failed";
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
	m_reader.reset();
	m_observer.reset();
}

int RISubscriber::setObserver(const std::shared_ptr<IRIObserver>& ob) {
	CHECK( ob );
	m_observer = ob;
	return 0;
}

int RISubscriber::defaultProcess(const std::shared_ptr<google::protobuf::Message>& msg,const std::string& topic) {
	LOG(WARNING) << "RISubscriber recv unexpected message: " << msg->GetTypeName() << ", from region: " << topic;
	return 0;
}

int RISubscriber::onRegion(const std::shared_ptr<google::protobuf::Message>& msg,const std::string& topic) {
	(void)topic;
	auto p = std::dynamic_pointer_cast<pub::PubRegion>(msg);
	CHECK(p);

	Region region(p->region());
	region.version = p->rt().version();

	DLOG(INFO) << "RISubscriber recv region: " << region.id << "(" << region.version << ")";
	m_observer->onRegion(region);
	return 0;
}

int RISubscriber::onRmRegion(const std::shared_ptr<google::protobuf::Message>& msg,const std::string& topic) {
	auto p = std::dynamic_pointer_cast<pub::PubRmRegion>(msg);
	CHECK(p);

	DLOG(INFO) << "RISubscriber recv rm region: " << topic;
	m_observer->onRmRegion(p->uuid());
	return 0;
}

int RISubscriber::onService(const std::shared_ptr<google::protobuf::Message>& msg,const std::string& topic) {
	auto p = std::dynamic_pointer_cast<pub::PubService>(msg);
	CHECK(p);

	Service svc(p->service());
	DLOG(INFO) << "RISubscriber recv service: " << svc.name << " in region:" << topic << "(" << p->rt().version() << ")";
	m_observer->onService(topic,p->rt().version(),svc);
	return 0;
}

int RISubscriber::onRmService(const std::shared_ptr<google::protobuf::Message>& msg,const std::string& topic) {
	auto p = std::dynamic_pointer_cast<pub::PubRmService>(msg);
	CHECK(p);

	DLOG(INFO) << "RISubscriber recv rm service: " << p->name() << " in region:" << topic << "(" << p->rt().version() << ")";
	m_observer->onRmService(topic,p->rt().version(),p->name());
	return 0;
}

int RISubscriber::onPayload(const std::shared_ptr<google::protobuf::Message>& msg,const std::string& topic) {
	auto p = std::dynamic_pointer_cast<pub::PubPayload>(msg);
	CHECK(p);

	Payload pl(p->payload());

	DLOG(INFO) << "RISubscriber recv payload: " << pl.id << " in region:" << topic << "(" << p->rt().version() << ")";
	m_observer->onPayload(topic,p->rt().version(),pl);
	return 0;
}

int RISubscriber::onRmPayload(const std::shared_ptr<google::protobuf::Message>& msg,const std::string& topic) {
	auto p = std::dynamic_pointer_cast<pub::PubRmPayload>(msg);
	CHECK(p);

	DLOG(INFO) << "RISubscriber recv rm payload: " << p->uuid() << " in region:" << topic << "(" << p->rt().version() << ")";
	m_observer->onRmPayload(topic,p->rt().version(),p->uuid());
	return 0;
}


