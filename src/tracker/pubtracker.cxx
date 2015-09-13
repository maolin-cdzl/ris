#include <glog/logging.h>
#include "tracker/pubtracker.h"


PubTracker::PubTracker(zloop_t* loop) :
	m_loop(loop),
	m_capacity(4)
{
}


PubTracker::~PubTracker() {
	stop();
}

int PubTracker::start(const std::shared_ptr<RITrackerTable>& table,size_t capacity) {
	if( m_factories.empty() ) {
		m_table = table;
		m_capacity = capacity;
		return 0;
	} else {
		return -1;
	}
}

void PubTracker::stop() {
	m_table.reset();
	m_factories.clear();
}

void PubTracker::onRegion(const Region& reg) {
	auto it = m_factories.find(reg.id);
	if( it != m_factories.end() ) {
		it->second->observer()->onRegion(reg);
	} else if( m_factories.size() < m_capacity ) {
		auto factory = std::make_shared<RegionFactory>(m_loop);
		if( 0 == factory->start(std::bind(&PubTracker::onFactoryCompleted,this,std::placeholders::_1,std::placeholders::_2),reg) ) {
			m_factories.insert( std::make_pair(reg.id,factory) );
		}
	}
}

void PubTracker::onRmRegion(const ri_uuid_t& reg) {
	auto it = m_factories.find(reg);
	if( it != m_factories.end() ) {
		m_factories.erase(it);
	}
}

void PubTracker::onService(const ri_uuid_t& reg,uint32_t version,const Service& svc) {
	auto it = m_factories.find(reg);
	if( it != m_factories.end() ) {
		it->second->observer()->onService(reg,version,svc);
	}
}

void PubTracker::onRmService(const ri_uuid_t& reg,uint32_t version,const std::string& svc) {
	auto it = m_factories.find(reg);
	if( it != m_factories.end() ) {
		it->second->observer()->onRmService(reg,version,svc);
	}
}

void PubTracker::onPayload(const ri_uuid_t& reg,uint32_t version,const Payload& pl) {
	auto it = m_factories.find(reg);
	if( it != m_factories.end() ) {
		it->second->observer()->onPayload(reg,version,pl);
	}
}

void PubTracker::onRmPayload(const ri_uuid_t& reg,uint32_t version,const ri_uuid_t& pl) {
	auto it = m_factories.find(reg);
	if( it != m_factories.end() ) {
		it->second->observer()->onRmPayload(reg,version,pl);
	}
}

void PubTracker::onFactoryCompleted(const ri_uuid_t& reg,const std::shared_ptr<IRegionLRUReader>& product) {
	auto it = m_factories.find(reg);
	CHECK(m_factories.end() != it);

	if( product ) {
		auto table = m_table.lock();
		if( table ) {
			table->merge(product);
		}
	}

	m_factories.erase(it);
}


