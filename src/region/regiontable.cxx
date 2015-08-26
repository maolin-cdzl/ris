#include <glog/logging.h>
#include "ris/region/regiontable.h"

RIRegionTable::RIRegionTable(const Region& reg) :
	m_region(reg),
	m_observer()
{
}

RIRegionTable::~RIRegionTable() {
}

void RIRegionTable::setObserver(IRIObserver* ob) {
	m_observer = ob;
}

void RIRegionTable::unsetObserver() {
	m_observer = nullptr;
}

int RIRegionTable::newService(const Service& svc) {
	if( m_services_idx.end() == m_services_idx.find(svc.id) ) {
		++m_region.version;
		ServiceRt svcrt(svc,ri_time_now(),m_region.version);
		auto it = m_services.insert(m_services.end(),svcrt);
		m_services_idx.insert( std::make_pair(svc.id,it) );

		LOG(INFO) << "new service: " << svc.id << "|" << svc.address << " version: " << m_region.version;
		if( m_observer != nullptr ) {
			m_observer->onNewService(m_region,svc);
		}
		return 0;
	} else {
		return -1;
	}
}

int RIRegionTable::delService(const uuid_t& svc) {
	auto it = m_services_idx.find(svc);
	if( it != m_services_idx.end() ) {
		auto itl = it->second;
		m_services_idx.erase(it);
		m_services.erase(itl);

		++m_region.version;

		LOG(INFO) << "remove service: " << svc  << " version: " << m_region.version;
		if( m_observer != nullptr ) {
			m_observer->onDelService(m_region,svc);
		}
		return 0;
	} else {
		return -1;
	}
}

int RIRegionTable::newPayload(const Payload& pl) {
	if( m_payloads_idx.end() == m_payloads_idx.find(pl.id) ) {
		++m_region.version;
		PayloadRt plrt(pl,ri_time_now(),m_region.version);
		auto it = m_payloads.insert(m_payloads.end(),plrt);
		m_payloads_idx.insert( std::make_pair(pl.id,it) );

		LOG(INFO) << "new payload: " << pl.id << " version: " << m_region.version;
		if( m_observer != nullptr ) {
			m_observer->onNewPayload(m_region,pl);
		}
		return 0;
	} else {
		return -1;
	}
}

int RIRegionTable::delPayload(const uuid_t& pl) {
	auto it = m_payloads_idx.find(pl);
	if( it != m_payloads_idx.end() ) {
		auto itl = it->second;
		m_payloads_idx.erase(it);
		m_payloads.erase(itl);

		++m_region.version;

		LOG(INFO) << "remove payload: " << pl << " version: " << m_region.version;
		if( m_observer != nullptr ) {
			m_observer->onDelPayload(m_region,pl);
		}
		return 0;
	} else {
		return -1;
	}
}

RIRegionTable::service_list_t RIRegionTable::update_timeouted_service(ri_time_t timeout,size_t maxcount) {
	service_list_t svcs;
	const ri_time_t now = ri_time_now();
	const ri_time_t expired = now - timeout;
	size_t count = 0;

	for( auto it = m_services.begin(); count < maxcount && it != m_services.end(); ++count ) {
		if( it->timeval <= expired ) {
			it->timeval = now;
			auto itv = it;
			++it;

			svcs.push_back(*itv);
			m_services.splice(m_services.end(),m_services,itv);
		} else {
			break;
		}
	}

	return std::move(svcs);
}

RIRegionTable::payload_list_t RIRegionTable::update_timeouted_payload(ri_time_t timeout,size_t maxcount) {
	payload_list_t pls;
	const ri_time_t now = ri_time_now();
	const ri_time_t expired = now - timeout;
	size_t count = 0;

	for( auto it = m_payloads.begin(); count < maxcount && it != m_payloads.end(); ++count) {
		if( it->timeval <= expired ) {
			it->timeval = now;
			auto itv = it;
			++it;

			pls.push_back(*itv);
			m_payloads.splice(m_payloads.end(),m_payloads,itv);
		} else {
			break;
		}
	}

	return std::move(pls);
}

std::shared_ptr<Snapshot> RIRegionTable::buildSnapshot() {
	std::shared_ptr<Snapshot> ss(new Snapshot());
	std::shared_ptr<SnapshotPartition> sp(m_region.toSnapshot());
	
	for(auto it=m_services.begin(); it != m_services.end(); ++it) {
		auto v = it->toSnapshot();
		sp->addItem(v);
	}

	for(auto it=m_payloads.begin(); it != m_payloads.end(); ++it) {
		auto v = it->toSnapshot();
		sp->addItem(v);
	}

	ss->addPartition(sp);
	return std::move(ss);
}



