#pragma once

#include <czmq.h>
#include <memory>
#include <string>
#include <list>

#include "ris/region/publisher.h"
#include "ris/region/regiontable.h"
#include "ris/snapshot/snapshotservice.h"

#include "ris/regionapi.pb.h"
#include "ris/region/regionctx.h"
#include "zmqx/zdispatcher.h"

class RIRegionActor {
public:
	RIRegionActor();
	~RIRegionActor();

	int start(const std::shared_ptr<RegionCtx>& ctx);
	int stop();

	inline std::shared_ptr<RegionCtx> getCtx() {
		return m_ctx;
	}
private:
	void run(zsock_t* pipe);
	int onPipeReadable(zsock_t* pipe);

	static void actorRunner(zsock_t* pipe,void* args);
	static int pipeReadableAdapter(zloop_t* loop,zsock_t* reader,void* arg);


	void defaultOpt(const std::shared_ptr<google::protobuf::Message>& msg,int err);
	void addService(const std::shared_ptr<google::protobuf::Message>& msg);
	void rmService(const std::shared_ptr<google::protobuf::Message>& msg);
	void addPayload(const std::shared_ptr<google::protobuf::Message>& msg);
	void rmPayload(const std::shared_ptr<google::protobuf::Message>& msg);

private:
	bool						m_running;
	zactor_t*					m_actor;
	zloop_t*					m_loop;
	zsock_t*					m_rep;

	std::shared_ptr<RegionCtx>			m_ctx;

	std::shared_ptr<RIRegionTable>		m_table;
	std::shared_ptr<RIPublisher>		m_pub;
	std::shared_ptr<SnapshotService>	m_ssvc;
	std::shared_ptr<Dispatcher>			m_disp;
};

