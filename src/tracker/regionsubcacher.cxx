#include <glog/logging.h>
#include "tracker/regionsubcacher.h"

RegionSubCacher::RegionSubCacher(const Region& region) :
	m_region_id(region.id)
{
	onRegion(region);
}

RegionSubCacher::~RegionSubCacher() {
}

void RegionSubCacher::onRegion(const Region& reg) {
	if( reg.id == m_region_id ) {
		m_updates.push(PubData::fromRegion(reg));
	} else {
		LOG(WARNING) << "RegionSubCacher recv unmatched pub, " << m_region_id << ":" << reg.id;
	}
}

void RegionSubCacher::onRmRegion(const ri_uuid_t& reg) {
	if( reg == m_region_id ) {
		decltype(m_updates)().swap(m_updates);
	} else {
		LOG(WARNING) << "RegionSubCacher recv unmatched pub, " << m_region_id << ":" << reg;
	}
}

void RegionSubCacher::onService(const ri_uuid_t& reg,uint32_t version,const Service& svc) {
	if( reg == m_region_id ) {
		m_updates.push(PubData::fromService(version,svc));
	} else {
		LOG(WARNING) << "RegionSubCacher recv unmatched pub, " << m_region_id << ":" << reg;
	}
}

void RegionSubCacher::onRmService(const ri_uuid_t& reg,uint32_t version,const ri_uuid_t& svc) {
	if( reg == m_region_id ) {
		m_updates.push(PubData::fromRmService(version,svc));
	} else {
		LOG(WARNING) << "RegionSubCacher recv unmatched pub, " << m_region_id << ":" << reg;
	}
}

void RegionSubCacher::onPayload(const ri_uuid_t& reg,uint32_t version,const Payload& pl) {
	if( reg == m_region_id ) {
		m_updates.push(PubData::fromPayload(version,pl));
	} else {
		LOG(WARNING) << "RegionSubCacher recv unmatched pub, " << m_region_id << ":" << reg;
	}
}

void RegionSubCacher::onRmPayload(const ri_uuid_t& reg,uint32_t version,const ri_uuid_t& pl) {
	if( reg == m_region_id ) {
		m_updates.push(PubData::fromRmPayload(version,pl));
	} else {
		LOG(WARNING) << "RegionSubCacher recv unmatched pub, " << m_region_id << ":" << reg;
	}
}

int RegionSubCacher::present(uint32_t version,const std::shared_ptr<IRIObserver>& observer) {
	// find EQ version
	bool found = false;
	while( ! m_updates.empty() ) {
		auto update = m_updates.front();
		m_updates.pop();
		if( update->version == version ) {
			found = true;
			break;
		}
		
		DLOG(INFO) << "RegionSubCacher drop old pub " << update->version;
	}

	if( ! found ) {
		LOG(ERROR) << "Region(" << m_region_id << ") pub has not contain snapshot version: " << version;
		return -1;
	}

	uint32_t expect_version = version + 1;
	while( ! m_updates.empty() ) {
		auto& update = m_updates.front();
		if( update->version != expect_version ) {
			LOG(ERROR) << "Region(" << m_region_id << ") pub is not continuously,expect: " << expect_version << ", got: " << update->version;
			break;
		}
		update->present(m_region_id,observer);
		m_updates.pop();
		++expect_version;
	};

	if( ! m_updates.empty() ) {
		// clear
		decltype(m_updates)().swap(m_updates);
		return -1;
	} else {
		return 0;
	}
}


