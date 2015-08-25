#pragma once

#include <string>
#include "ris/ritypes.h"

class IRIObserver {
public:
	virtual void onNewRegion(const Region& reg) = 0;
	virtual void onDelRegion(const uuid_t& reg) = 0;
	virtual void onNewService(const RegionRt& reg,const Service& svc) = 0;
	virtual void onDelService(const RegionRt& reg,const uuid_t& svc) = 0;
	virtual void onNewPayload(const RegionRt& reg,const Payload& pl) = 0;
	virtual void onDelPayload(const RegionRt& reg,const uuid_t& pl) = 0;
};

