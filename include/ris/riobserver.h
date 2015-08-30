#pragma once

#include <string>
#include "ris/ritypes.h"

class IRIObserver {
public:
	virtual void onRegion(const Region& reg) = 0;
	virtual void onRmRegion(const uuid_t& reg) = 0;
	virtual void onService(const uuid_t& reg,uint32_t version,const Service& svc) = 0;
	virtual void onRmService(const uuid_t& reg,uint32_t version,const std::string& svc) = 0;
	virtual void onPayload(const uuid_t& reg,uint32_t version,const Payload& pl) = 0;
	virtual void onRmPayload(const uuid_t& reg,uint32_t version,const uuid_t& pl) = 0;
};

