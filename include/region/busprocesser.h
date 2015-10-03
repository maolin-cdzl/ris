#pragma once

#include <string>
#include <unordered_map>
#include <list>
#include <memory>
#include <czmq.h>

#include "zmqx/zenvelope.h"
#include "zmqx/zloopreader.h"
#include "ris/bus.pb.h"
#include "region/regionctx.h"
#include "region/regiontable.h"


class BusProcesser {
public:
	BusProcesser(zloop_t* loop);
	~BusProcesser();

	int start(const std::shared_ptr<RIRegionTable>& table,const std::shared_ptr<RegionCtx>& ctx);
	void stop();

private:
	int onBusReadable(zsock_t* sock);
	int onWorkerReadable(zsock_t* sock);

	int sendto_worker(zmsg_t** p_msg);

	void push_worker(const std::string& worker);
	void pop_worker();
private:
	zloop_t*							m_loop;
	std::shared_ptr<RIRegionTable>		m_table;
	size_t								m_hwm;
	std::shared_ptr<ZLoopReader>		m_bus_reader;
	std::shared_ptr<ZLoopReader>		m_worker_reader;
	std::list<zmsg_t*>					m_msgs;

	std::list<std::string>				m_workers;
	std::unordered_map<std::string,std::list<std::string>::iterator>	m_workers_index;
};

