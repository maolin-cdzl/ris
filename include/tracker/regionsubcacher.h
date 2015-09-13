#pragma once

#include <queue>
#include "ris/riobserver.h"
#include "tracker/pubdata.h"


class RegionSubCacher : public IRIObserver {
public:
	RegionSubCacher(const Region& region);
	virtual ~RegionSubCacher();

	int present(uint32_t version,const std::shared_ptr<IRIObserver>& observer);
protected:
	virtual void onRegion(const Region& reg);
	virtual void onRmRegion(const ri_uuid_t& reg);
	virtual void onService(const ri_uuid_t& reg,uint32_t version,const Service& svc);
	virtual void onRmService(const ri_uuid_t& reg,uint32_t version,const ri_uuid_t& svc);
	virtual void onPayload(const ri_uuid_t& reg,uint32_t version,const Payload& pl);
	virtual void onRmPayload(const ri_uuid_t& reg,uint32_t version,const ri_uuid_t& pl);

private:
	ri_uuid_t								m_region_id;
	std::queue<std::shared_ptr<PubData>>	m_updates;
};

