#pragma once

#include "ris/ritypes.h"


class ISnapshotBuilder {
public:
	virtual int addRegion(const Region& region) = 0;
	virtual int addService(const ri_uuid_t& region,const Service& svc) = 0;
	virtual int addPayload(const ri_uuid_t& region,const Payload& pl) = 0;
	virtual void onCompleted(int err) = 0;
};

