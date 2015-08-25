#include "ris/pubsub/publisher.h"


RIPublisher::RIPublisher() :
	m_pub(nullptr) {
	}


RIPublisher::~RIPublisher() {
	shutdown();
}


int RIPublisher::start(const std::list<std::string>& brokers) {
	int result = -1;
	assert( ! brokers.empty() );
	do {
		if( nullptr != m_pub )
			break;
		m_pub = zsock_new(ZMQ_PUB);
		if( nullptr == m_pub )
			break;
		m_brokers = brokers;

		result = 0;
		for( auto it = m_brokers.begin(); it != m_brokers.end(); ++it) {
			if( -1 == zsock_connect(m_pub,"%s",it->c_str()) ) {
				result = -1;
				m_brokers.clear();
				break;
			}
		}
	} while( 0 );

	if( result == -1 ) {
		if( m_pub ) {
			zsock_destroy(&m_pub);
		}
	}

	return result;
}

int RIPublisher::shutdown() {
	if( m_pub ) {
		zsock_destroy(&m_pub);
		m_brokers.clear();
		return 0;
	} else {
		return -1;
	}
}

int RIPublisher::pubRegion(const RegionRt& region) {
	zmsg_t* msg = region.toPublish();
	return zmsg_send(&msg,m_pub);
}

int RIPublisher::pubRemoveRegion(const uuid_t& reg) {
	zmsg_t* msg = RegionRt::toPublishDel(reg);
	return zmsg_send(&msg,m_pub);
}

int RIPublisher::pubService(const RegionRt& region,const Service& svc) {
	zmsg_t* msg = svc.toPublish(region);
	return zmsg_send(&msg,m_pub);
}

int RIPublisher::pubRemoveService(const RegionRt& region,const uuid_t& svc) {
	zmsg_t* msg = Service::toPublishDel(region,svc);
	return zmsg_send(&msg,m_pub);
}

int RIPublisher::pubPayload(const RegionRt& region,const Payload& pl) {
	zmsg_t* msg = pl.toPublish(region);
	return zmsg_send(&msg,m_pub);
}

int RIPublisher::pubRemovePayload(const RegionRt& region,const uuid_t& pl) {
	zmsg_t* msg = Payload::toPublishDel(region,pl);
	return zmsg_send(&msg,m_pub);
}


