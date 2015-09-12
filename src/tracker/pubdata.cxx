#include <glog/logging.h>
#include "tracker/pubdata.h"


PubData::PubData() :
	version(0),
	type(UNKNOWN),
	data(nullptr)
{
}

PubData::PubData(uint32_t v,UpdateType t,void* d) :
	version(v),
	type(t),
	data(d)
{
}

PubData::~PubData() {
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
				LOG(FATAL) << "PubData unknown type: " << (int)type;
				break;
		}
	}
}

void PubData::present(const ri_uuid_t& region,const std::shared_ptr<IRIObserver>& ob) {
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
			LOG(FATAL) << "PubData present unknown type: " << (int)type;
			break;
	}
}

std::shared_ptr<PubData> PubData::fromRegion(const Region& reg) {
	return std::make_shared<PubData>(reg.version,UPDATE_REGION,new Region(reg));
}

std::shared_ptr<PubData> PubData::fromService(uint32_t version,const Service& svc) {
	return std::make_shared<PubData>(version,UPDATE_SERVICE,new Service(svc));
}

std::shared_ptr<PubData> PubData::fromPayload(uint32_t version,const Payload& pl) {
	return std::make_shared<PubData>(version,UPDATE_PAYLOAD,new Payload(pl));
}

std::shared_ptr<PubData> PubData::fromRmService(uint32_t version,const std::string& svc) {
	return std::make_shared<PubData>(version,RM_SERVICE,new std::string(svc));
}

std::shared_ptr<PubData> PubData::fromRmPayload(uint32_t version,const ri_uuid_t& pl) {
	return std::make_shared<PubData>(version,RM_PAYLOAD,new ri_uuid_t(pl));
}

