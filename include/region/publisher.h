#pragma once

#include <list>
#include <czmq.h>

#include "ris/ritypes.h"
#include "ris/riobserver.h"
#include "region/regiontable.h"

class RIPublisher : public IRIObserver {
public:
	RIPublisher(zloop_t* loop);
	virtual ~RIPublisher();

	int start(const std::string& pubaddr);
	int stop();


	int pubRegion(const Region& region);
	int pubRemoveRegion(const ri_uuid_t& reg);

	int pubService(const ri_uuid_t& region,uint32_t version,const Service& svc);
	int pubRemoveService(const ri_uuid_t& region,uint32_t version,const ri_uuid_t& svc);

	int pubPayload(const ri_uuid_t& ri_uuid_t,uint32_t version,const Payload& pl);
	int pubRemovePayload(const ri_uuid_t& region,uint32_t version,const ri_uuid_t& pl);
private:
	virtual void onRegion(const Region& reg);
	virtual void onRmRegion(const ri_uuid_t& reg);
	virtual void onService(const ri_uuid_t& reg,uint32_t version,const Service& svc);
	virtual void onRmService(const ri_uuid_t& reg,uint32_t version,const ri_uuid_t& svc);
	virtual void onPayload(const ri_uuid_t& reg,uint32_t version,const Payload& pl);
	virtual void onRmPayload(const ri_uuid_t& reg,uint32_t version,const ri_uuid_t& pl);
private:
	zloop_t*						m_loop;
	zsock_t*						m_pub;
private:
	RIPublisher(const RIPublisher&) = delete;
	RIPublisher(const RIPublisher&&) = delete;
	RIPublisher& operator = (const RIPublisher&) = delete;
};
