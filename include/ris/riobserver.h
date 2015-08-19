#pragma once

#include <string>
#include "ris/ritypes.h"

class IRIObserver {
public:
	virtual void onNewRegion(const Region& reg) = 0;
	virtual void onDelRegion(const std::string& reg) = 0;
	virtual void onNewService(const uuid_t& reg,uint32_t version,const Service& svc);
	virtual void onDelService(const uuid_t& reg,uint32_t version,const uuid_t& svc);
	virtual void onNewPayload(const uuid_t& reg,uint32_t version,const Payload& pl);
	virtual void onDelPayload(const uuid_t& reg,uint32_t version,const uuid_t& pl);
};

