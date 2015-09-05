#pragma once

#include "snapshot/snapshotable.h"
#include "snapshot/snapshotserviceworker.h"
#include "zmqx/zloopreader.h"


class SnapshotService {
public:
	SnapshotService(zloop_t* loop);

	~SnapshotService();

	int start(const std::shared_ptr<ISnapshotable>& snapshotable,const std::string& svcAddress,const std::string& workerAddress,size_t capacity=4);
	int stop();
private:
	int onRepReadable(zsock_t* reader);
	void onWorkerDone(const std::shared_ptr<SnapshotServiceWorker>& worker,int err);
private:
	zloop_t*						m_loop;
	std::shared_ptr<ISnapshotable>	m_snapshotable;
	std::string						m_svc_address;
	std::string						m_worker_address;
	size_t							m_capacity;

	ZLoopReader						m_rep_reader;
	std::list<std::shared_ptr<SnapshotServiceWorker>>			m_workers;
};

