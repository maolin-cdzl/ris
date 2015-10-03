#pragma once

#include <unordered_map>
#include <czmq.h>
#include "snapshot/snapshotctx.h"
#include "snapshot/snapshotable.h"
#include "snapshot/snapshotserviceworker.h"
#include "zmqx/zloopreader.h"
#include "zmqx/zpbreader.h"


class SnapshotService {
public:
	SnapshotService();
	~SnapshotService();

	int start(const std::shared_ptr<ISnapshotable>& snapshotable,const std::shared_ptr<SnapshotCtx>& ctx);
	void stop();
private:
	int onSnapshotReq(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,const std::shared_ptr<ZEnvelope>& envelop);
	int onSyncSignal(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,const std::shared_ptr<ZEnvelope>& envelop);

	std::shared_ptr<envelope_dispatcher_t> make_dispatcher();

	int onPipeReadable(zsock_t* reader);
	int onTimer();
	void run(zsock_t* pipe);
	static void actorAdapter(zsock_t* pipe,void* arg);
private:
	bool							m_running;
	zactor_t*						m_actor;
	std::shared_ptr<ISnapshotable>	m_snapshotable;
	std::shared_ptr<SnapshotCtx>	m_ctx;

	std::unordered_map<std::string,std::shared_ptr<SnapshotServiceWorker>>			m_workers;
};

