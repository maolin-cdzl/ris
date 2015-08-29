#include <glog/logging.h>
#include "ris/region/publisher.h"
#include "zmqx/zprotobuf++.h"

static const long RI_PUB_REPEAT_SECOND			= 60;
static const long RI_PUB_REPEAT_MS				= RI_PUB_REPEAT_SECOND * 1000;
static const long RI_PUB_REPEAT_TIMER			= 20;
static const long RI_PUB_REPEAT_LOOP_TIMER_COUNT	= RI_PUB_REPEAT_MS / RI_PUB_REPEAT_TIMER;

static const long RI_PUB_REGION_SECOND			= 5;
static const long RI_PUB_REGION_MS				= RI_PUB_REGION_SECOND * 1000;

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


int RIPublisher::start(const std::shared_ptr<RIRegionTable>& table,const std::string& pubaddr) {
	int result = -1;
	assert( ! pubaddr.empty() );

	LOG(INFO) << "RIPublisher initialize...";
	do {
		if( nullptr != m_pub )
			break;

		m_pub = zsock_new(ZMQ_PUB);
		assert(m_pub);
		if( -1 == zsock_connect(m_pub,pubaddr.c_str()) ) {
			LOG(ERROR) << "RIPublisher can NOT connect to: " << pubaddr;
			break;
		}
		
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

	if( m_pub ) {
		zsock_destroy(&m_pub);
	}
	LOG(INFO) << "RIPublisher shutdown";
	return 0;
}

int RIPublisher::startLoop(zloop_t* loop) {
	m_tid_reg = zloop_timer(loop,RI_PUB_REGION_MS,0,onRegionPubTimer,this);
	if( -1 == m_tid_reg )
		return -1;
	m_tid_repeat = zloop_timer(loop,RI_PUB_REPEAT_TIMER,0,onRepeatPubTimer,this);
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

void RIPublisher::onRegion(const Region& reg) {
}

void RIPublisher::onRmRegion(const uuid_t& reg) {
}

void RIPublisher::onService(const uuid_t& reg,uint32_t version,const Service& svc) {
	pubService(reg,version,svc);
}

void RIPublisher::onRmService(const uuid_t& reg,uint32_t version,const uuid_t& svc) {
	pubRemoveService(reg,version,svc);
}

void RIPublisher::onPayload(const uuid_t& reg,uint32_t version,const Payload& pl) {
	pubPayload(reg,version,pl);
}

void RIPublisher::onRmPayload(const uuid_t& reg,uint32_t version,const uuid_t& pl) {
	pubRemovePayload(reg,version,pl);
}

int RIPublisher::pubRegion(const Region& region) {
	LOG(INFO) << "pub region: " << region.id;
	auto p = region.toPublish();
	return zpb_send(m_pub,*p);
}

int RIPublisher::pubRemoveRegion(const uuid_t& reg) {
	LOG(INFO) << "pub region offline: " << reg;
	auto p = Region::toPublishRm(reg);
	if( -1 != zpb_send(m_pub,*p) ) {
		zsock_flush(m_pub);
		return 0;
	} else {
		return -1;
	}
}

int RIPublisher::pubService(const uuid_t& region,uint32_t version,const Service& svc) {
	auto p = svc.toPublish(region,version);
	return zpb_send(m_pub,*p);
}

int RIPublisher::pubRemoveService(const uuid_t& region,uint32_t version,const std::string& svc) {
	auto p = Service::toPublishRm(region,version,svc);
	return zpb_send(m_pub,*p);
}

int RIPublisher::pubPayload(const uuid_t& region,uint32_t version,const Payload& pl) {
	auto p = pl.toPublish(region,version);
	return zpb_send(m_pub,*p);
}

int RIPublisher::pubRemovePayload(const uuid_t& region,uint32_t version,const uuid_t& pl) {
	auto p = Payload::toPublishRm(region,version,pl);
	return zpb_send(m_pub,*p);
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

	auto svclist = m_table->update_timeouted_service(RI_PUB_REPEAT_MS,100);
	for(auto it=svclist.begin(); it != svclist.end(); ++it) {
		pubService(region.id,region.version,*it);
	}

	const size_t maxcount = (m_table->payload_size() / RI_PUB_REPEAT_LOOP_TIMER_COUNT) + 100;

	auto pldlist = m_table->update_timeouted_payload(RI_PUB_REPEAT_MS,maxcount);
	for(auto it=pldlist.begin(); it != pldlist.end(); ++it) {
		pubPayload(region.id,region.version,*it);
	}

	if( ! svclist.empty() || ! pldlist.empty() ) {
		LOG(INFO) << "Repeated pub " << svclist.size() << " service and " << pldlist.size() << " payload, version: " << region.version;
	}

	return 0;
}
