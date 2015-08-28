#pragma once

#include <czmq.h>
#include "ris/snapshot/snapshotable.h"

class SnapshotServiceWorker {
public:
	SnapshotServiceWorker(const std::string& address);
	~SnapshotServiceWorker();

	inline zactor_t* actor() const {
		return m_actor;
	}

	inline std::string endpoint() const {
		return m_endpoint;
	}

	int start(const snapshot_package_t& snapshot);
	int stop();
private:
	static void actorAdapterFn(zsock_t* pipe,void* arg);

	void run(zsock_t* pipe);
	zsock_t* createPipelineSock();
	int onPipeReadable(zsock_t* pipe);
	int onPipelineWritable(zsock_t* sock);

private:
	const std::string						m_address;						
	zactor_t*								m_actor;
	std::string								m_endpoint;

	snapshot_package_t						m_snapshot;
};

