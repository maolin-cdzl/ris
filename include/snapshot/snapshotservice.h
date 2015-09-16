#pragma once

#include <unordered_map>
#include <czmq.h>
#include "snapshot/snapshotable.h"
#include "snapshot/snapshotserviceworker.h"
#include "zmqx/zloopreader.h"
#include "zmqx/zpbreader.h"


class SnapshotService {
public:
	SnapshotService();
	~SnapshotService();

	int start(const std::shared_ptr<ISnapshotable>& snapshotable,const std::string& address,size_t capacity=4,size_t period_count=100,ri_time_t timeout=3000);
	void stop();
private:
	int onSnapshotReq(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,std::unique_ptr<ZEnvelope>& envelop);
	int onSyncSignal(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,std::unique_ptr<ZEnvelope>& envelop);

	std::shared_ptr<envelope_dispatcher_t> make_dispatcher();

	int onPipeReadable(zsock_t* reader);
	int onTimer();
	void run(zsock_t* pipe);
	static void actorAdapter(zsock_t* pipe,void* arg);
private:
	bool							m_running;
	zactor_t*						m_actor;
	std::shared_ptr<ISnapshotable>	m_snapshotable;
	std::string						m_address;
	size_t							m_capacity;
	size_t							m_period_count;
	ri_time_t						m_tv_timeout;

	std::unordered_map<std::string,std::shared_ptr<SnapshotServiceWorker>>			m_workers;
};

