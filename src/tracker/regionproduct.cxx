#include "tracker/regionproduct.h"

RegionProduct::RegionProduct() {
}

RegionProduct::~RegionProduct() {
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
	if( m_region.id == region ) {
		m_services.push_back(svc);
		return 0;
	} else {
		return -1;
	}
}

int RegionProduct::addPayload(const ri_uuid_t& region,const Payload& pl) {
	if( m_region.id == region ) {
		m_payloads.push_back(pl);
		return 0;
	} else {
		return -1;
	}
}

void RegionProduct::onCompleted(int err) {
	if( -1 == err ) {
		m_region.id.clear();
		m_services.clear();
		m_payloads.clear();
	}
}


