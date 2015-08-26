#include <glog/logging.h>
#include "ris/region/publisher.h"


RIPublisher::RIPublisher(zloop_t* loop) :
	m_loop(loop),
	m_pub(nullptr),
	m_tid_reg(-1),
	m_tid_repeat(-1)
{
}


RIPublisher::~RIPublisher() {
	stop();
}


int RIPublisher::start(const std::shared_ptr<RIRegionTable>& table,const std::list<std::string>& brokers) {
	int result = -1;
	assert( ! brokers.empty() );

	LOG(INFO) << "RIPublisher initialize...";
	do {
		if( nullptr != m_pub )
			break;
		m_brokers = brokers;

		m_pub = connectBrokers();
		if( nullptr == m_pub )
			break;
		m_table = table;
		m_table->setObserver(this);

		result = startLoop(m_loop);
	} while( 0 );

	if( result == -1 ) {
		LOG(ERROR) << "RIPublisher initialize error!";
		stopLoop(m_loop);
		if( m_pub ) {
			zsock_destroy(&m_pub);
		}
		if( m_table ) {
			m_table->unsetObserver();
			m_table = nullptr;
		}
		m_brokers.clear();
	} else {
		LOG(INFO) << "RIPublisher initialize done";
		pubRegion( m_table->region() );
	}

	return result;
}

int RIPublisher::stop() {
	if( nullptr == m_pub ) {
		return -1;
	}

	stopLoop(m_loop);
	if( m_table ) {
		pubRemoveRegion( m_table->region().id );
	}

	if( m_table ) {
		m_table->unsetObserver();
		m_table = nullptr;
	}
	m_brokers.clear();

	if( m_pub ) {
		zsock_destroy(&m_pub);
	}
	LOG(INFO) << "RIPublisher shutdown";
	return 0;
}

int RIPublisher::startLoop(zloop_t* loop) {
	m_tid_reg = zloop_timer(loop,5000,0,onRegionPubTimer,this);
	if( -1 == m_tid_reg )
		return -1;
	m_tid_repeat = zloop_timer(loop,20,0,onRepeatPubTimer,this);
	if( -1 == m_tid_repeat ) {
		zloop_timer_end(m_loop,m_tid_reg);
		m_tid_reg = -1;
		return -1;
	}
	return 0;
}

void RIPublisher::stopLoop(zloop_t* loop) {
	if( m_tid_reg != -1 ) {
		zloop_timer_end(loop,m_tid_reg);
		m_tid_reg = -1;
	}
	if( m_tid_repeat != -1 ) {
		zloop_timer_end(loop,m_tid_repeat);
		m_tid_repeat = -1;
	}
}

void RIPublisher::onNewRegion(const Region& reg) {
}

void RIPublisher::onDelRegion(const uuid_t& reg) {
}

void RIPublisher::onNewService(const RegionRt& reg,const Service& svc) {
	pubService(reg,svc);
}

void RIPublisher::onDelService(const RegionRt& reg,const uuid_t& svc) {
	pubRemoveService(reg,svc);
}

void RIPublisher::onNewPayload(const RegionRt& reg,const Payload& pl) {
	pubPayload(reg,pl);
}

void RIPublisher::onDelPayload(const RegionRt& reg,const uuid_t& pl) {
	pubRemovePayload(reg,pl);
}

int RIPublisher::pubRegion(const RegionRt& region) {
	LOG(INFO) << "pub region: " << region.id;
	zmsg_t* msg = region.toPublish();
	return zmsg_send(&msg,m_pub);
}

int RIPublisher::pubRemoveRegion(const uuid_t& reg) {
	LOG(INFO) << "pub region offline: " << reg;
	zmsg_t* msg = RegionRt::toPublishDel(reg);
	if( -1 != zmsg_send(&msg,m_pub) ) {
		zsock_flush(m_pub);
		return 0;
	} else {
		return -1;
	}
}

int RIPublisher::pubService(const RegionRt& region,const Service& svc) {
	LOG(INFO) << "pub service: " << svc.id << " version: " << region.version;
	zmsg_t* msg = svc.toPublish(region);
	return zmsg_send(&msg,m_pub);
}

int RIPublisher::pubRemoveService(const RegionRt& region,const uuid_t& svc) {
	LOG(INFO) << "pub service offline: " << svc  << " version: " << region.version;
	zmsg_t* msg = Service::toPublishDel(region,svc);
	return zmsg_send(&msg,m_pub);
}

int RIPublisher::pubPayload(const RegionRt& region,const Payload& pl) {
	LOG(INFO) << "pub payload: " << pl.id << " version: " << region.version;
	zmsg_t* msg = pl.toPublish(region);
	return zmsg_send(&msg,m_pub);
}

int RIPublisher::pubRemovePayload(const RegionRt& region,const uuid_t& pl) {
	LOG(INFO) << "pub payload offline: " << pl << " version: " << region.version;
	zmsg_t* msg = Payload::toPublishDel(region,pl);
	return zmsg_send(&msg,m_pub);
}


zsock_t* RIPublisher::connectBrokers() {
	zsock_t* sock = nullptr;
	assert( ! m_brokers.empty() );
	do {
		sock = zsock_new(ZMQ_PUB);
		if( nullptr == sock )
			break;

		for( auto it = m_brokers.begin(); it != m_brokers.end(); ++it) {
			if( -1 == zsock_connect(sock,"%s",it->c_str()) ) {
				LOG(ERROR) << "RIPublisher can not connect: " << *it;
				zsock_destroy(&sock);
				break;
			}
			LOG(INFO) << "pub connect broker: " << *it;
		}
	} while( 0 );

	return sock;
}

int RIPublisher::onRegionPubTimer(zloop_t *loop, int timer_id, void *arg) {
	RIPublisher* self = (RIPublisher*)arg;
	return self->pubRegion( self->m_table->region() );
}

int RIPublisher::onRepeatPubTimer(zloop_t *loop, int timer_id, void *arg) {
	RIPublisher* self = (RIPublisher*)arg;
	return self->pubRepeated();
}


int RIPublisher::pubRepeated() {
	auto region = m_table->region();

	auto svclist = m_table->update_timeouted_service(60000);
	for(auto it=svclist.begin(); it != svclist.end(); ++it) {
		pubService(region,*it);
	}

	auto pldlist = m_table->update_timeouted_payload(60000);
	for(auto it=pldlist.begin(); it != pldlist.end(); ++it) {
		pubPayload(region,*it);
	}

	return 0;
}
