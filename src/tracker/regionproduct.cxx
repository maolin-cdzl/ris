#include "tracker/regionproduct.h"

RegionProduct::RegionProduct() {
}

RegionProduct::~RegionProduct() {
}

const Region& RegionProduct::getRegion() const {
	return m_region;
}

const std::list<Service>& RegionProduct::getServicesOrderByLRU() const {
	return m_services;
}

const std::list<Payload>& RegionProduct::getPayloadsOrderByLRU() const {
	return m_payloads;
}


int RegionProduct::addRegion(const Region& region) {
	if( m_region.id.empty() || m_region.id == region.id ) {
		m_region = region;
		return 0;
	} else {
		return -1;
	}
}

int RegionProduct::addService(const ri_uuid_t& region,const Service& svc) {
	if( m_region.id == region && m_services_index.end() == m_services_index.find(svc.name) ) {
		m_services.push_back(svc);
		m_services_index.insert( std::make_pair(svc.name,std::prev(m_services.end())) );
		return 0;
	} else {
		return -1;
	}
}

int RegionProduct::addPayload(const ri_uuid_t& region,const Payload& pl) {
	if( m_region.id == region && m_payloads_index.end() == m_payloads_index.find(pl.id) ) {
		m_payloads.push_back(pl);
		m_payloads_index.insert( std::make_pair(pl.id,std::prev(m_payloads.end())) );
		return 0;
	} else {
		return -1;
	}
}

void RegionProduct::onCompleted(int err) {
	if( -1 == err ) {
		m_services_index.clear();
		m_payloads_index.clear();
		m_services.clear();
		m_payloads.clear();
		m_region.id.clear();
	}
}


void RegionProduct::onRegion(const Region& region) {
	if( region.id == m_region.id ) {
		m_region = region;
	}
}

void RegionProduct::onRmRegion(const ri_uuid_t& region) {
	if( m_region.id == region ) {
		m_services_index.clear();
		m_payloads_index.clear();
		m_services.clear();
		m_payloads.clear();
		m_region.id.clear();
	}
}

void RegionProduct::onService(const ri_uuid_t& region,uint32_t version,const Service& svc) {
	if( m_region.id != region ) {
		return;
	}

	m_region.version = version;
	auto iit = m_services_index.find(svc.name);
	if( iit == m_services_index.end() ) {
		m_services.push_back(svc);
		m_services_index.insert( std::make_pair(svc.name,std::prev(m_services.end())) );
	} else {
		auto cit = iit->second;
		*cit = svc;
		m_services.splice(m_services.end(),m_services,cit);
	}
}

void RegionProduct::onRmService(const ri_uuid_t& region,uint32_t version,const std::string& svc) {
	if( m_region.id != region ) {
		return;
	}
	m_region.version = version;
	auto iit = m_services_index.find(svc);
	if( iit != m_services_index.end() ) {
		auto cit = iit->second;
		m_services_index.erase(iit);
		m_services.erase(cit);
	}
}

void RegionProduct::onPayload(const ri_uuid_t& region,uint32_t version,const Payload& pl) {
	if( m_region.id != region ) {
		return;
	}
	m_region.version = version;
	auto iit = m_payloads_index.find(pl.id);
	if( iit == m_payloads_index.end() ) {
		m_payloads.push_back(pl);
		m_payloads_index.insert( std::make_pair(pl.id,std::prev(m_payloads.end())) );
	} else {
		auto cit = iit->second;
		*cit = pl;
		m_payloads.splice(m_payloads.end(),m_payloads,cit);
	}
}

void RegionProduct::onRmPayload(const ri_uuid_t& region,uint32_t version,const ri_uuid_t& pl) {
	if( m_region.id != region ) {
		return;
	}
	m_region.version = version;
	auto iit = m_payloads_index.find(pl);
	if( iit != m_payloads_index.end() ) {
		auto cit = iit->second;
		m_payloads_index.erase(iit);
		m_payloads.erase(cit);
	}
}

