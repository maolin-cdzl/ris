#include <glog/logging.h>
#include "tracker/regionsubcacher.h"

RegionSubCacher::RegionSubCacher(const ri_uuid_t& region_id) :
	m_region_id(region_id)
{
}

RegionSubCacher::~RegionSubCacher() {
}

void RegionSubCacher::onRegion(const Region& reg) {
	if( reg.id == m_region_id ) {
		m_updates.push(PubData::fromRegion(reg));
	}
}

void RegionSubCacher::onRmRegion(const ri_uuid_t& reg) {
	if( reg == m_region_id ) {
		decltype(m_updates)().swap(m_updates);
	}
}

void RegionSubCacher::onService(const ri_uuid_t& reg,uint32_t version,const Service& svc) {
	if( reg == m_region_id ) {
		m_updates.push(PubData::fromService(version,svc));
	}
}

void RegionSubCacher::onRmService(const ri_uuid_t& reg,uint32_t version,const ri_uuid_t& svc) {
	if( reg == m_region_id ) {
		m_updates.push(PubData::fromRmService(version,svc));
	}
}

void RegionSubCacher::onPayload(const ri_uuid_t& reg,uint32_t version,const Payload& pl) {
	if( reg == m_region_id ) {
		m_updates.push(PubData::fromPayload(version,pl));
	}
}

void RegionSubCacher::onRmPayload(const ri_uuid_t& reg,uint32_t version,const ri_uuid_t& pl) {
	if( reg == m_region_id ) {
		m_updates.push(PubData::fromRmPayload(version,pl));
	}
}

int RegionSubCacher::present(uint32_t version,const std::shared_ptr<IRIObserver>& observer) {

	// find EQ version
	while( ! m_updates.empty() ) {
		auto update = m_updates.front();
		m_updates.pop();
		if( update->version == version )
			break;
	}

	if( m_updates.empty() ) {
		LOG(ERROR) << "Region(" << m_region_id << ") pub has not contain snapshot version: " << version;
		return -1;
	}

	uint32_t expect_version = version + 1;
	do {
		auto& update = m_updates.front();
		if( update->version != expect_version ) {
			LOG(ERROR) << "Region(" << m_region_id << ") pub is not continuously,expect: " << expect_version << ", got: " << update->version;
			break;
		}
		update->present(m_region_id,observer);
		m_updates.pop();
		++expect_version;
	} while( ! m_updates.empty() );

	if( ! m_updates.empty() ) {
		// clear
		decltype(m_updates)().swap(m_updates);
		return -1;
	} else {
		return 0;
	}
}


