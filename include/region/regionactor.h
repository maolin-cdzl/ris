#pragma once

#include <czmq.h>
#include <memory>
#include <string>
#include <list>

#include "ris/regionapi.pb.h"
#include "zmqx/zdispatcher.h"
#include "region/publisher.h"
#include "region/regiontable.h"
#include "region/regionctx.h"
#include "snapshot/snapshotservice.h"

class RIRegionActor {
public:
	RIRegionActor();
	~RIRegionActor();

	int start(const std::shared_ptr<RegionCtx>& ctx);
	int stop();

	// wait actor shutdown,it will block caller
	int wait();

	inline std::shared_ptr<RegionCtx> getCtx() {
		return m_ctx;
	}
private:
	void run(zsock_t* pipe);
	int onPipeReadable(zsock_t* pipe);
	std::shared_ptr<Dispatcher> make_dispatcher(ZDispatcher& zdisp);

	static void actorRunner(zsock_t* pipe,void* args);

	int defaultOpt(ZDispatcher& zdisp,const std::shared_ptr<google::protobuf::Message>& msg);
	int addService(ZDispatcher& zdisp,const std::shared_ptr<google::protobuf::Message>& msg);
	int rmService(ZDispatcher& zdisp,const std::shared_ptr<google::protobuf::Message>& msg);
	int addPayload(ZDispatcher& zdisp,const std::shared_ptr<google::protobuf::Message>& msg);
	int rmPayload(ZDispatcher& zdisp,const std::shared_ptr<google::protobuf::Message>& msg);
	int handshake(ZDispatcher& zdisp,const std::shared_ptr<google::protobuf::Message>& msg);

private:
	bool						m_running;
	zactor_t*					m_actor;
	zloop_t*					m_loop;

	std::shared_ptr<RegionCtx>			m_ctx;
	std::shared_ptr<RIRegionTable>		m_table;
};

