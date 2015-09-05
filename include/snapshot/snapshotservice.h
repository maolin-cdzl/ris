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
	static int workerReaderAdapter(zloop_t* loop,zsock_t* reader,void* arg);

	int onRepReadable(zsock_t* reader);
	int onWorkerReadable(zloop_t* loop,zsock_t* reader);
	zactor_t* createWorker();

	std::shared_ptr<SnapshotServiceWorker> popWorker(zsock_t* sock);
private:
	zloop_t*						m_loop;
	std::shared_ptr<ISnapshotable>	m_snapshotable;
	std::string						m_svc_address;
	std::string						m_worker_address;
	size_t							m_capacity;

	ZLoopReader						m_rep_reader;
	std::list<std::shared_ptr<SnapshotServiceWorker>>			m_workers;
};

