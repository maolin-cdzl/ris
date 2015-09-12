#include "tracker/regionsubcacher.h"

RegionSubCacher::RegionSubCacher(const ri_uuid_t& region_id) :
	m_region_id(region_id)
{
}

RegionSubCacher::~RegionSubCacher() {
}

void RegionSubCacher::onRegion(const Region& reg) {
	if( reg.id == m_region_id ) {
		m_updates.push_back(PubData::fromRegion(reg));
	}
}

void RegionSubCacher::onRmRegion(const ri_uuid_t& reg) {
	if( reg == m_region_id ) {
		m_updates.clear();
	}
}

void RegionSubCacher::onService(const ri_uuid_t& reg,uint32_t version,const Service& svc) {
	if( reg == m_region_id ) {
		m_updates.push_back(PubData::fromService(version,svc));
	}
}

void RegionSubCacher::onRmService(const ri_uuid_t& reg,uint32_t version,const ri_uuid_t& svc) {
	if( reg == m_region_id ) {
		m_updates.push_back(PubData::fromRmService(version,svc));
	}
}

void RegionSubCacher::onPayload(const ri_uuid_t& reg,uint32_t version,const Payload& pl) {
	if( reg == m_region_id ) {
		m_updates.push_back(PubData::fromPayload(version,pl));
	}
}

void RegionSubCacher::onRmPayload(const ri_uuid_t& reg,uint32_t version,const ri_uuid_t& pl) {
	if( reg == m_region_id ) {
		m_updates.push_back(PubData::fromRmPayload(version,pl));
	}
}

