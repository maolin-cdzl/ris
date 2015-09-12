#pragma once

#include "snapshot/snapshotbuilder.h"

class RegionFactory : public ISnapshotBuilder {
public:
	RegionFactory();
	virtual ~RegionFactory();

protected:
	virtual int addRegion(const Region& region);
	virtual int addService(const ri_uuid_t& region,const Service& svc);
	virtual int addPayload(const ri_uuid_t& region,const Payload& pl);
	virtual void onCompleted(int err);

private:
	Region					m_region;
	std::list<Service>		m_services;
	std::list<Payload>		m_payloads;
};

