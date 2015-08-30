#include <glog/logging.h>
#include "ris/tracker/subcacher.h"


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
				delete (uuid_t*)data;
				break;
			default:
				LOG(FATAL) << "UpdateData unknown type: " << (int)type;
				break;
		}
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

std::shared_ptr<UpdateData> UpdateData::fromRmPayload(uint32_t version,const uuid_t& pl) {
	return std::make_shared<UpdateData>(version,RM_PAYLOAD,new uuid_t(pl));
}


/**
 * class SubCacher
 */

SubCacher::SubCacher(const std::function<void(const Region&)>& fnNewRegion,const std::function<void(const uuid_t&)>& fnRmRegion) :
	m_fn_new_region(fnNewRegion),
	m_fn_rm_region(fnRmRegion)
{
}

SubCacher::~SubCacher() {
}

void SubCacher::onRegion(const Region& reg) {
	auto update = UpdateData::fromRegion(reg);
	auto it = m_updates.find(reg.id);
	if( it == m_updates.end() ) {
		it = m_updates.insert( std::make_pair(reg.id,std::list<std::shared_ptr<UpdateData>>()) ).first;
		m_fn_new_region(reg);
	}

	it->second.push_back(update);
}

void SubCacher::onRmRegion(const uuid_t& reg) {
	auto it = m_updates.find(reg);
	if( it != m_updates.end() ) {
		m_updates.erase(it);
		m_fn_rm_region(reg);
	}
}

void SubCacher::onService(const uuid_t& reg,uint32_t version,const Service& svc) {
	auto it = m_updates.find(reg);
	if( it != m_updates.end() ) {
		auto update = UpdateData::fromService(version,svc);
		it->second.push_back(update);
	}
}

void SubCacher::onRmService(const uuid_t& reg,uint32_t version,const uuid_t& svc) {
	auto it = m_updates.find(reg);
	if( it != m_updates.end() ) {
		auto update = UpdateData::fromRmService(version,svc);
		it->second.push_back(update);
	}
}

void SubCacher::onPayload(const uuid_t& reg,uint32_t version,const Payload& pl) {
	auto it = m_updates.find(reg);
	if( it != m_updates.end() ) {
		auto update = UpdateData::fromPayload(version,pl);
		it->second.push_back(update);
	}
}

void SubCacher::onRmPayload(const uuid_t& reg,uint32_t version,const uuid_t& pl) {
	auto it = m_updates.find(reg);
	if( it != m_updates.end() ) {
		auto update = UpdateData::fromRmPayload(version,pl);
		it->second.push_back(update);
	}
}

