#include "ris/region/regiontable.h"

RIRegionTable::RIRegionTable(const Region& reg) :
	m_region(reg),
	m_observer(),
	m_version(0)
{
}

RIRegionTable::~RIRegionTable() {
}

void RIRegionTable::setObserver(const std::shared_ptr<IRIObserver>& ob) {
	m_observer = ob;
}

void RIRegionTable::unsetObserver() {
	m_observer.reset();
}

uint32_t RIRegionTable::newService(const Service& svc) {
	if( m_services_idx.end() == m_services_idx.find(svc.id) ) {
		ServiceRt svcrt;
		*(Service*)&svcrt = svc;
		svcrt.timeval = ri_time_now();
		auto it = m_services.insert(m_services.end(),svcrt);
		m_services_idx.insert( std::make_pair(svc.id,it) );

		++m_version;

		if( m_observer != nullptr ) {
			m_observer->onNewService(m_region.id,m_version,svc);
		}
	}
	return m_version;
}

uint32_t RIRegionTable::delService(const uuid_t& svc) {
	auto it = m_services_idx.find(svc);
	if( it != m_services_idx.end() ) {
		auto itl = it->second;
		m_services_idx.erase(it);
		m_services.erase(itl);

		++m_version;

		if( m_observer != nullptr ) {
			m_observer->onDelService(m_region.id,m_version,svc);
		}
	}
	return m_version;
}

uint32_t RIRegionTable::newPayload(const Payload& pl) {
	if( m_payloads_idx.end() == m_payloads_idx.find(pl.id) ) {
		PayloadRt plrt;
		*(Payload*)&plrt = pl;
		plrt.timeval = ri_time_now();
		auto it = m_payloads.insert(m_payloads.end(),plrt);
		m_payloads_idx.insert( std::make_pair(pl.id,it) );

		++m_version;

		if( m_observer != nullptr ) {
			m_observer->onNewPayload(m_region.id,m_version,pl);
		}
	}
	return m_version;
}

uint32_t RIRegionTable::delPayload(const uuid_t& pl) {
	auto it = m_payloads_idx.find(pl);
	if( it != m_payloads_idx.end() ) {
		auto itl = it->second;
		m_payloads_idx.erase(it);
		m_payloads.erase(itl);

		++m_version;

		if( m_observer != nullptr ) {
			m_observer->onDelPayload(m_region.id,m_version,pl);
		}
	}
	return m_version;
}


