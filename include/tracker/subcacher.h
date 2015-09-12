#pragma once

#include <functional>
#include <unordered_map>
#include "ris/ritypes.h"
#include "ris/riobserver.h"
#include "tracker/pubdata.h"

class SubCacher : public IRIObserver {
public:
	SubCacher(const std::function<void(const Region&)>& fnNewRegion,const std::function<void(const ri_uuid_t&)>& fnRmRegion);
	virtual ~SubCacher();

	int present(const ri_uuid_t& region,uint32_t version,const std::shared_ptr<IRIObserver>& observer);
private:
	virtual void onRegion(const Region& reg);
	virtual void onRmRegion(const ri_uuid_t& reg);
	virtual void onService(const ri_uuid_t& reg,uint32_t version,const Service& svc);
	virtual void onRmService(const ri_uuid_t& reg,uint32_t version,const ri_uuid_t& svc);
	virtual void onPayload(const ri_uuid_t& reg,uint32_t version,const Payload& pl);
	virtual void onRmPayload(const ri_uuid_t& reg,uint32_t version,const ri_uuid_t& pl);

private:

private:
	std::function<void(const Region&)>						m_fn_new_region;
	std::function<void(const ri_uuid_t&)>						m_fn_rm_region;
	std::unordered_map<ri_uuid_t,std::list<std::shared_ptr<PubData>>>		m_updates;
};

