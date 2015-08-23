#pragma once

#include <czmq.h>
#include "ris/snapshot/snapshot.h"

class SnapshotServiceWorker {
public:
	SnapshotServiceWorker(const std::string& express);
	~SnapshotServiceWorker();

	inline zactor_t* actor() const {
		return m_actor;
	}

	inline std::string endpoint() const {
		return m_endpoint;
	}

	int start(std::shared_ptr<Snapshot>& snapshot);
	int stop();
private:
	static void actorAdapterFn(zsock_t* pipe,void* arg);

	void run(zsock_t* pipe);
	zsock_t* createPipelineSock();
	int onPipeReadable(zsock_t* pipe);
	int onPipelineWritable(zsock_t* sock);

	int transSnapshot(std::shared_ptr<Snapshot>& snapshot);
	int transSnapshotPartition(std::shared_ptr<SnapshotPartition>& part);

private:
	const std::string						m_express;						
	zactor_t*								m_actor;
	std::string								m_endpoint;
	std::list<zmsg_t*>						m_msgs;
};

