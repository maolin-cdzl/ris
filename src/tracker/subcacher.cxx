#include <glog/logging.h>
#include "tracker/subcacher.h"


UpdateData::UpdateData() :
	version(0),
	type(UNKNOWN),
	data(nullptr)
{
}

UpdateData::UpdateData(uint32_t v,UpdateType t,void* d) :
	version(v),
	type(t),
	data(d)
{
}

UpdateData::~UpdateData() {
	if( data ) {
		switch( type )
		{
			case UPDATE_REGION :
				delete (Region*)data;
				break;
			case UPDATE_SERVICE :
				delete (Service*)data;
				break;
			case RM_SERVICE :
				delete (std::string*)data;
				break;
			case UPDATE_PAYLOAD :
				delete (Payload*)data;
				break;
			case RM_PAYLOAD :
				delete (ri_uuid_t*)data;
				break;
			default:
				LOG(FATAL) << "UpdateData unknown type: " << (int)type;
				break;
		}
	}
}

void UpdateData::present(const ri_uuid_t& region,const std::shared_ptr<IRIObserver>& ob) {
	assert( type != UNKNOWN );
	assert( data );

	switch( type )
	{
		case UPDATE_REGION :
			ob->onRegion(*(Region*)data);
			break;
		case UPDATE_SERVICE :
			ob->onService(region,version,*(Service*)data);
			break;
		case RM_SERVICE :
			ob->onRmService(region,version,*(std::string*)data);
			break;
		case UPDATE_PAYLOAD :
			ob->onPayload(region,version,*(Payload*)data);
			break;
		case RM_PAYLOAD :
			ob->onRmPayload(region,version,*(ri_uuid_t*)data);
			break;
		default:
			LOG(FATAL) << "UpdateData present unknown type: " << (int)type;
			break;
	}
}

std::shared_ptr<UpdateData> UpdateData::fromRegion(const Region& reg) {
	return std::make_shared<UpdateData>(reg.version,UPDATE_REGION,new Region(reg));
}

std::shared_ptr<UpdateData> UpdateData::fromService(uint32_t version,const Service& svc) {
	return std::make_shared<UpdateData>(version,UPDATE_SERVICE,new Service(svc));
}

std::shared_ptr<UpdateData> UpdateData::fromPayload(uint32_t version,const Payload& pl) {
	return std::make_shared<UpdateData>(version,UPDATE_PAYLOAD,new Payload(pl));
}

std::shared_ptr<UpdateData> UpdateData::fromRmService(uint32_t version,const std::string& svc) {
	return std::make_shared<UpdateData>(version,RM_SERVICE,new std::string(svc));
}

std::shared_ptr<UpdateData> UpdateData::fromRmPayload(uint32_t version,const ri_uuid_t& pl) {
	return std::make_shared<UpdateData>(version,RM_PAYLOAD,new ri_uuid_t(pl));
}


/**
 * class SubCacher
 */

SubCacher::SubCacher(const std::function<void(const Region&)>& fnNewRegion,const std::function<void(const ri_uuid_t&)>& fnRmRegion) :
	m_fn_new_region(fnNewRegion),
	m_fn_rm_region(fnRmRegion)
{
}

SubCacher::~SubCacher() {
}

void SubCacher::onRegion(const Region& reg) {
	DLOG(INFO) << "SubCacher recv pub region: " << reg.id << "(" << reg.version << ")";
	auto update = UpdateData::fromRegion(reg);
	auto it = m_updates.find(reg.id);
	if( it == m_updates.end() ) {
		LOG(INFO) << "SubCacher found new region: " << reg.id << ",version: " << reg.version;
		it = m_updates.insert( std::make_pair(reg.id,std::list<std::shared_ptr<UpdateData>>()) ).first;
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
		auto update = UpdateData::fromService(version,svc);
		it->second.push_back(update);
	}
}

void SubCacher::onRmService(const ri_uuid_t& reg,uint32_t version,const std::string& svc) {
	DLOG(INFO) << "SubCacher recv pub remove service: " << svc << " provided by region: " << reg << "(" << version << ")";
	auto it = m_updates.find(reg);
	if( it != m_updates.end() ) {
		auto update = UpdateData::fromRmService(version,svc);
		it->second.push_back(update);
	}
}

void SubCacher::onPayload(const ri_uuid_t& reg,uint32_t version,const Payload& pl) {
	DLOG(INFO) << "SubCacher recv pub payload: " << pl.id << " provided by region: " << reg << "(" << version << ")";
	auto it = m_updates.find(reg);
	if( it != m_updates.end() ) {
		auto update = UpdateData::fromPayload(version,pl);
		it->second.push_back(update);
	}
}

void SubCacher::onRmPayload(const ri_uuid_t& reg,uint32_t version,const ri_uuid_t& pl) {
	DLOG(INFO) << "SubCacher recv pub rm payload: " << pl << " provided by region: " << reg << "(" << version << ")";
	auto it = m_updates.find(reg);
	if( it != m_updates.end() ) {
		auto update = UpdateData::fromRmPayload(version,pl);
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

