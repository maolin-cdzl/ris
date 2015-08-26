#pragma once

#include <czmq.h>
#include <memory>
#include <string>
#include <list>

#include "ris/pubsub/publisher.h"
#include "ris/region/regiontable.h"
#include "ris/snapshot/snapshotservice.h"

class RIRegionActor {
public:
	RIRegionActor();
	~RIRegionActor();

	int start(const std::string& conf);
	int stop();

private:
	int loadConfig(const std::string& conf);
	int onPipeReadable(zsock_t* pipe);

	static void actorRunner(zsock_t* pipe,void* args);
	static int pipeReadableAdapter(zloop_t* loop,zsock_t* reader,void* arg);
private:
	bool						m_running;
	zactor_t*					m_actor;

	Region						m_region;
	std::string					m_snapshot_worker_address;
	std::list<std::string>		m_brokers;					

	std::shared_ptr<RIRegionTable>		m_table;
	std::shared_ptr<RIPublisher>		m_pub;
	std::shared_ptr<SnapshotService>	m_ssvc;
};

