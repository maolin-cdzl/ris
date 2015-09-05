#include <glog/logging.h>
#include "region/publisher.h"
#include "zmqx/zprotobuf++.h"

RIPublisher::RIPublisher(zloop_t* loop) :
	m_loop(loop),
	m_pub(nullptr)
{
}


RIPublisher::~RIPublisher() {
	stop();
}


int RIPublisher::start(const std::string& pubaddr,bool bind) {
	int result = -1;
	assert( ! pubaddr.empty() );

	LOG(INFO) << "RIPublisher initialize...";
	do {
		if( nullptr != m_pub )
			break;

		m_pub = zsock_new(ZMQ_PUB);
		assert(m_pub);
		if( bind ) {
			if( -1 == zsock_bind(m_pub,"%s",pubaddr.c_str()) ) {
				LOG(ERROR) << "RIPublisher can NOT bind to: " << pubaddr;
				break;
			}
		} else {
			if( -1 == zsock_connect(m_pub,"%s",pubaddr.c_str()) ) {
				LOG(ERROR) << "RIPublisher can NOT connect to: " << pubaddr;
				break;
			}
		}
		
		result = 0;
	} while( 0 );

	if( result == -1 ) {
		LOG(ERROR) << "RIPublisher initialize error!";
		if( m_pub ) {
			zsock_destroy(&m_pub);
		}
	} else {
		LOG(INFO) << "RIPublisher initialize done";
	}

	return result;
}

int RIPublisher::stop() {
	if( nullptr == m_pub ) {
		return -1;
	}

	if( m_pub ) {
		zsock_destroy(&m_pub);
	}
	LOG(INFO) << "RIPublisher shutdown";
	return 0;
}

void RIPublisher::onRegion(const Region& reg) {
	pubRegion(reg);
}

void RIPublisher::onRmRegion(const ri_uuid_t& reg) {
	pubRemoveRegion(reg);
}

void RIPublisher::onService(const ri_uuid_t& reg,uint32_t version,const Service& svc) {
	pubService(reg,version,svc);
}

void RIPublisher::onRmService(const ri_uuid_t& reg,uint32_t version,const std::string& svc) {
	pubRemoveService(reg,version,svc);
}

void RIPublisher::onPayload(const ri_uuid_t& reg,uint32_t version,const Payload& pl) {
	pubPayload(reg,version,pl);
}

void RIPublisher::onRmPayload(const ri_uuid_t& reg,uint32_t version,const ri_uuid_t& pl) {
	pubRemovePayload(reg,version,pl);
}

int RIPublisher::pubRegion(const Region& region) {
	LOG(INFO) << "pub region: " << region.id;
	auto p = region.toPublish();
	return zpb_send(m_pub,*p);
}

int RIPublisher::pubRemoveRegion(const ri_uuid_t& reg) {
	LOG(INFO) << "pub region offline: " << reg;
	auto p = Region::toPublishRm(reg);
	if( -1 != zpb_send(m_pub,*p) ) {
		zsock_flush(m_pub);
		return 0;
	} else {
		return -1;
	}
}

int RIPublisher::pubService(const ri_uuid_t& region,uint32_t version,const Service& svc) {
	auto p = svc.toPublish(region,version);
	return zpb_send(m_pub,*p);
}

int RIPublisher::pubRemoveService(const ri_uuid_t& region,uint32_t version,const std::string& svc) {
	auto p = Service::toPublishRm(region,version,svc);
	return zpb_send(m_pub,*p);
}

int RIPublisher::pubPayload(const ri_uuid_t& region,uint32_t version,const Payload& pl) {
	auto p = pl.toPublish(region,version);
	return zpb_send(m_pub,*p);
}

int RIPublisher::pubRemovePayload(const ri_uuid_t& region,uint32_t version,const ri_uuid_t& pl) {
	auto p = Payload::toPublishRm(region,version,pl);
	return zpb_send(m_pub,*p);
}

