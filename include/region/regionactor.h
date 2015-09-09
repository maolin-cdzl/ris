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
	std::shared_ptr<Dispatcher> make_dispatcher(zsock_t* reader);

	static void actorRunner(zsock_t* pipe,void* args);

	int defaultOpt(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg,int err);
	int addService(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg);
	int rmService(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg);
	int addPayload(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg);
	int rmPayload(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg);
	int handshake(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg);

private:
	bool						m_running;
	zactor_t*					m_actor;
	zloop_t*					m_loop;

	std::shared_ptr<RegionCtx>			m_ctx;
	std::shared_ptr<RIRegionTable>		m_table;
	std::shared_ptr<RIPublisher>		m_pub;
	std::shared_ptr<SnapshotService>	m_ssvc;
	std::shared_ptr<Dispatcher>			m_disp;
};

