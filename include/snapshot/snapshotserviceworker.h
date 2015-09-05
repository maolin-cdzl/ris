#pragma once

#include <functional>
#include <czmq.h>
#include "snapshot/snapshotable.h"
#include "zmqx/zloopreader.h"

class SnapshotServiceWorker {
public:
	SnapshotServiceWorker(zloop_t* loop);
	~SnapshotServiceWorker();

	inline zactor_t* actor() const {
		return m_actor;
	}

	inline std::string endpoint() const {
		return m_endpoint;
	}

	int start(const std::string& address,const snapshot_package_t& snapshot,const std::function<void(int)>& cb);
	int stop();
private:
	static void actorAdapterFn(zsock_t* pipe,void* arg);

	void run(zsock_t* pipe);
	zsock_t* createPipelineSock();
	int onPipeReadable(zsock_t* sock,zsock_t* pipe);
	int onPipelineWritable(zsock_t* sock,zsock_t* pipe);
	int onActorReadable(zsock_t* sock);
private:
	zactor_t*								m_actor;
	std::string								m_address;						
	std::string								m_endpoint;
	std::function<void(int)>				m_cb;
	ZLoopReader								m_reader;
	snapshot_package_t						m_snapshot;
};

