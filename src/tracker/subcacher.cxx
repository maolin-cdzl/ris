#include <glog/logging.h>
#include "tracker/subcacher.h"



SubCacher::SubCacher(const std::function<void(const Region&)>& fnNewRegion,const std::function<void(const ri_uuid_t&)>& fnRmRegion) :
	m_fn_new_region(fnNewRegion),
	m_fn_rm_region(fnRmRegion)
{
}

SubCacher::~SubCacher() {
}

void SubCacher::onRegion(const Region& reg) {
	DLOG(INFO) << "SubCacher recv pub region: " << reg.id << "(" << reg.version << ")";
	auto update = PubData::fromRegion(reg);
	auto it = m_updates.find(reg.id);
	if( it == m_updates.end() ) {
		LOG(INFO) << "SubCacher found new region: " << reg.id << ",version: " << reg.version;
		it = m_updates.insert( std::make_pair(reg.id,std::list<std::shared_ptr<PubData>>()) ).first;
		m_fn_new_region(reg);
	}

	it->second.push_back(update);
}

void SubCacher::onRmRegion(const ri_uuid_t& reg) {
	DLOG(INFO) << "SubCacher recv pub rm region: " << reg;
	auto it = m_updates.find(reg);
	if( it != m_updates.end() ) {
		LOG(INFO) << "SubCacher remove region: " << reg;
		m_updates.erase(it);
		m_fn_rm_region(reg);
	}
}

void SubCacher::onService(const ri_uuid_t& reg,uint32_t version,const Service& svc) {
	DLOG(INFO) << "SubCacher recv pub service: " << svc.name << " provided by region: " << reg << "(" << version << ")";
	auto it = m_updates.find(reg);
	if( it != m_updates.end() ) {
		auto update = PubData::fromService(version,svc);
		it->second.push_back(update);
	}
}

void SubCacher::onRmService(const ri_uuid_t& reg,uint32_t version,const std::string& svc) {
	DLOG(INFO) << "SubCacher recv pub remove service: " << svc << " provided by region: " << reg << "(" << version << ")";
	auto it = m_updates.find(reg);
	if( it != m_updates.end() ) {
		auto update = PubData::fromRmService(version,svc);
		it->second.push_back(update);
	}
}

void SubCacher::onPayload(const ri_uuid_t& reg,uint32_t version,const Payload& pl) {
	DLOG(INFO) << "SubCacher recv pub payload: " << pl.id << " provided by region: " << reg << "(" << version << ")";
	auto it = m_updates.find(reg);
	if( it != m_updates.end() ) {
		auto update = PubData::fromPayload(version,pl);
		it->second.push_back(update);
	}
}

void SubCacher::onRmPayload(const ri_uuid_t& reg,uint32_t version,const ri_uuid_t& pl) {
	DLOG(INFO) << "SubCacher recv pub rm payload: " << pl << " provided by region: " << reg << "(" << version << ")";
	auto it = m_updates.find(reg);
	if( it != m_updates.end() ) {
		auto update = PubData::fromRmPayload(version,pl);
		it->second.push_back(update);
	}
}

int SubCacher::present(const ri_uuid_t& region,uint32_t version,const std::shared_ptr<IRIObserver>& observer) {
	auto it = m_updates.find(region);
	if( it == m_updates.end() )
		return -1;

	auto& l = it->second;
	while( ! l.empty() ) {
		auto& update = l.front();
		if( update->version > version ) {
			update->present(it->first,observer);
		}
		l.pop_front();
	}
	m_updates.erase(it);
	return 0;
}

