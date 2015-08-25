#pragma once

#include <list>
#include <czmq.h>

#include "ris/ritypes.h"
#include "ris/loopable.h"
#include "ris/riobserver.h"
#include "ris/region/regiontable.h"

class RIPublisher : public ILoopable, public IRIObserver {
public:
	RIPublisher(zloop_t* loop);
	~RIPublisher();

	int start(const std::shared_ptr<RIRegionTable>& table, const std::list<std::string>& brokers);
	int stop();


	int pubRegion(const RegionRt& region);
	int pubRemoveRegion(const uuid_t& reg);

	int pubService(const RegionRt& region,const Service& svc);
	int pubRemoveService(const RegionRt& region,const uuid_t& svc);

	int pubPayload(const RegionRt& region,const Payload& pl);
	int pubRemovePayload(const RegionRt& region,const uuid_t& pl);
private:
	virtual int startLoop(zloop_t* loop);
	virtual void stopLoop(zloop_t* loop);

	virtual void onNewRegion(const Region& reg);
	virtual void onDelRegion(const uuid_t& reg);
	virtual void onNewService(const RegionRt& reg,const Service& svc);
	virtual void onDelService(const RegionRt& reg,const uuid_t& svc);
	virtual void onNewPayload(const RegionRt& reg,const Payload& pl);
	virtual void onDelPayload(const RegionRt& reg,const uuid_t& pl);
private:
	zsock_t* connectBrokers();

	int pubRepeated();
	static int onRegionPubTimer(zloop_t *loop, int timer_id, void *arg);
	static int onRepeatPubTimer(zloop_t *loop, int timer_id, void *arg);
private:
	zloop_t*						m_loop;
	zsock_t*						m_pub;
	std::list<std::string>			m_brokers;
	std::shared_ptr<RIRegionTable>	m_table;
	int								m_tid_reg;
	int								m_tid_repeat;
private:
	RIPublisher(const RIPublisher&) = delete;
	RIPublisher(const RIPublisher&&) = delete;
	RIPublisher& operator = (const RIPublisher&) = delete;
};
