#pragma once

#include <czmq.h>
#include <memory>
#include <string>
#include <list>

#include "zmqx/zpbreader.h"
#include "ris/regionapi.pb.h"
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
	std::shared_ptr<envelope_dispatcher_t> make_dispatcher();

	static void actorRunner(zsock_t* pipe,void* args);

	int defaultOpt(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,std::unique_ptr<ZEnvelope>& envelope);
	int addService(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,std::unique_ptr<ZEnvelope>& envelope);
	int rmService(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,std::unique_ptr<ZEnvelope>& envelope);
	int addPayload(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,std::unique_ptr<ZEnvelope>& envelope);
	int rmPayload(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,std::unique_ptr<ZEnvelope>& envelope);
	int handshake(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,std::unique_ptr<ZEnvelope>& envelope);

private:
	bool						m_running;
	zactor_t*					m_actor;
	zloop_t*					m_loop;

	std::shared_ptr<RegionCtx>			m_ctx;
	std::shared_ptr<RIRegionTable>		m_table;
};

