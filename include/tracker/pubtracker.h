#pragma once

#include <memory>
#include <unordered_map>
#include <czmq.h>
#include "ris/riobserver.h"
#include "tracker/regionfactory.h"
#include "tracker/trackertable.h"

class PubTracker : public IRIObserver {
public:
	PubTracker(zloop_t* loop);
	virtual ~PubTracker();

	int start(const std::shared_ptr<RITrackerTable>& table,size_t capacity=4);
	void stop();
protected:
	virtual void onRegion(const Region& reg);
	virtual void onRmRegion(const ri_uuid_t& reg);
	virtual void onService(const ri_uuid_t& reg,uint32_t version,const Service& svc);
	virtual void onRmService(const ri_uuid_t& reg,uint32_t version,const std::string& svc);
	virtual void onPayload(const ri_uuid_t& reg,uint32_t version,const Payload& pl);
	virtual void onRmPayload(const ri_uuid_t& reg,uint32_t version,const ri_uuid_t& pl);

private:
	void onFactoryCompleted(const ri_uuid_t& reg,const std::shared_ptr<IRegionLRUReader>& product);
private:
	zloop_t*								m_loop;
	std::unordered_map<ri_uuid_t,std::shared_ptr<RegionFactory>>		m_factories;
	std::weak_ptr<RITrackerTable>			m_table;
	size_t									m_capacity;
};

