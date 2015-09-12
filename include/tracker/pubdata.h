#pragma once

#include "ris/ritypes.h"
#include "ris/riobserver.h"

class PubData {
public:
	enum UpdateType {
		UNKNOWN					= 0,
		UPDATE_REGION,
		UPDATE_SERVICE,
		RM_SERVICE,
		UPDATE_PAYLOAD,
		RM_PAYLOAD,
	};
	uint32_t			version;
	UpdateType			type;
	void*				data;

	PubData();
	PubData(uint32_t v,UpdateType t,void* d);
	~PubData();

	void present(const ri_uuid_t& region,const std::shared_ptr<IRIObserver>& ob);

	static std::shared_ptr<PubData> fromRegion(const Region& reg);
	static std::shared_ptr<PubData> fromService(uint32_t version,const Service& svc);
	static std::shared_ptr<PubData> fromPayload(uint32_t version,const Payload& pl);
	static std::shared_ptr<PubData> fromRmService(uint32_t version,const std::string& svc);
	static std::shared_ptr<PubData> fromRmPayload(uint32_t version,const ri_uuid_t& pl);
};

