#pragma once

#include <string>
#include "ris/ritypes.h"

class IRIObserver {
public:
	virtual void onRegion(const Region& reg) = 0;
	virtual void onRmRegion(const ri_uuid_t& reg) = 0;
	virtual void onService(const ri_uuid_t& reg,uint32_t version,const Service& svc) = 0;
	virtual void onRmService(const ri_uuid_t& reg,uint32_t version,const std::string& svc) = 0;
	virtual void onPayload(const ri_uuid_t& reg,uint32_t version,const Payload& pl) = 0;
	virtual void onRmPayload(const ri_uuid_t& reg,uint32_t version,const ri_uuid_t& pl) = 0;
};

