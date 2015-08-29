#include <glog/logging.h>
#include "ris/region/regiontable.h"

RIRegionTable::RIRegionTable(const std::shared_ptr<RegionCtx>& ctx) :
	m_observer(nullptr)
{
	m_region.id = ctx->uuid;
	m_region.idc = ctx->idc;
	m_region.bus_address = ctx->bus_address;
	m_region.snapshot_address = ctx->snapshot_svc_address;
	m_region.version = 0;
	m_region.timeval = 0;
}

RIRegionTable::~RIRegionTable() {
}

void RIRegionTable::setObserver(IRIObserver* ob) {
	m_observer = ob;
}

void RIRegionTable::unsetObserver() {
	m_observer = nullptr;
}

int RIRegionTable::addService(const std::string& name,const std::string& address) {
	if( m_services_idx.end() == m_services_idx.find(name) ) {
		++m_region.version;
		Service svc(name,address,ri_time_now());
		auto it = m_services.insert(m_services.end(),svc);
		m_services_idx.insert( std::make_pair(name,it) );

		LOG(INFO) << "new service: " << name << "|" << address << " version: " << m_region.version;
		if( m_observer != nullptr ) {
			m_observer->onService(m_region.id,m_region.version,svc);
		}
		return 0;
	} else {
		return -1;
	}
}

int RIRegionTable::rmService(const std::string& svc) {
	auto it = m_services_idx.find(svc);
	if( it != m_services_idx.end() ) {
		auto itl = it->second;
		m_services_idx.erase(it);
		m_services.erase(itl);

		++m_region.version;

		LOG(INFO) << "remove service: " << svc  << " version: " << m_region.version;
		if( m_observer != nullptr ) {
			m_observer->onRmService(m_region.id,m_region.version,svc);
		}
		return 0;
	} else {
		return -1;
	}
}

int RIRegionTable::addPayload(const uuid_t& pl) {
	if( m_payloads_idx.end() == m_payloads_idx.find(pl) ) {
		++m_region.version;
		Payload payload(pl,ri_time_now());
		auto it = m_payloads.insert(m_payloads.end(),payload);
		m_payloads_idx.insert( std::make_pair(pl,it) );

		LOG(INFO) << "new payload: " << pl << " version: " << m_region.version;
		if( m_observer != nullptr ) {
			m_observer->onPayload(m_region.id,m_region.version,payload);
		}
		return 0;
	} else {
		return -1;
	}
}

int RIRegionTable::rmPayload(const uuid_t& pl) {
	auto it = m_payloads_idx.find(pl);
	if( it != m_payloads_idx.end() ) {
		auto itl = it->second;
		m_payloads_idx.erase(it);
		m_payloads.erase(itl);

		++m_region.version;

		LOG(INFO) << "remove payload: " << pl << " version: " << m_region.version;
		if( m_observer != nullptr ) {
			m_observer->onPayload(m_region.id,m_region.version,pl);
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

snapshot_package_t RIRegionTable::buildSnapshot() {
	snapshot_package_t package;
	package.push_back(std::shared_ptr<snapshot::SnapshotBegin>(new snapshot::SnapshotBegin()));

	package.push_back(m_region.toSnapshotBegin());
	
	for(auto it=m_services.begin(); it != m_services.end(); ++it) {
		package.push_back(it->toSnapshot());
	}

	for(auto it=m_payloads.begin(); it != m_payloads.end(); ++it) {
		package.push_back(it->toSnapshot());
	}

	package.push_back(m_region.toSnapshotEnd());
	package.push_back(std::shared_ptr<snapshot::SnapshotEnd>(new snapshot::SnapshotEnd()));
	return std::move(package);
}



