#pragma once

#include <list>
#include <czmq.h>

#include "ris/ritypes.h"
#include "ris/riobserver.h"
#include "ris/region/regiontable.h"

class RIPublisher : public IRIObserver {
public:
	RIPublisher(zloop_t* loop);
	virtual ~RIPublisher();

	int start(const std::shared_ptr<RIRegionTable>& table, const std::string& pubaddr);
	int stop();


	int pubRegion(const Region& region);
	int pubRemoveRegion(const uuid_t& reg);

	int pubService(const Region& region,const Service& svc);
	int pubRemoveService(const Region& region,const uuid_t& svc);

	int pubPayload(const Region& region,const Payload& pl);
	int pubRemovePayload(const Region& region,const uuid_t& pl);
private:
	int startLoop(zloop_t* loop);
	void stopLoop(zloop_t* loop);

	virtual void onNewRegion(const Region& reg);
	virtual void onDelRegion(const uuid_t& reg);
	virtual void onNewService(const Region& reg,const Service& svc);
	virtual void onDelService(const Region& reg,const uuid_t& svc);
	virtual void onNewPayload(const Region& reg,const Payload& pl);
	virtual void onDelPayload(const Region& reg,const uuid_t& pl);
private:
	int pubRepeated();
	static int onRegionPubTimer(zloop_t *loop, int timer_id, void *arg);
	static int onRepeatPubTimer(zloop_t *loop, int timer_id, void *arg);
private:
	zloop_t*						m_loop;
	zsock_t*						m_pub;
	std::shared_ptr<RIRegionTable>	m_table;
	int								m_tid_reg;
	int								m_tid_repeat;
private:
	RIPublisher(const RIPublisher&) = delete;
	RIPublisher(const RIPublisher&&) = delete;
	RIPublisher& operator = (const RIPublisher&) = delete;
};
