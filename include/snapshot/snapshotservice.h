#pragma once

#include "snapshot/snapshotable.h"
#include "snapshot/snapshotserviceworker.h"


class SnapshotService {
public:
	SnapshotService(zloop_t* loop);

	~SnapshotService();

	int start(const std::shared_ptr<ISnapshotable>& snapshotable,const std::string& svcAddress,const std::string& workerAddress,size_t capacity=4);
	int stop();
private:
	int startLoop(zloop_t* loop);
	void stopLoop(zloop_t* loop);

	static int mainReaderAdapter(zloop_t* loop,zsock_t* reader,void* arg);
	static int workerReaderAdapter(zloop_t* loop,zsock_t* reader,void* arg);

	int onMainReadable(zloop_t* loop);
	int onWorkerReadable(zloop_t* loop,zsock_t* reader);
	zactor_t* createWorker();

	std::shared_ptr<SnapshotServiceWorker> popWorker(zsock_t* sock);
private:
	zloop_t*						m_loop;
	std::shared_ptr<ISnapshotable>	m_snapshotable;
	std::string						m_svc_address;
	std::string						m_worker_address;
	size_t							m_capacity;

	zsock_t*						m_sock;
	std::list<std::shared_ptr<SnapshotServiceWorker>>			m_workers;
};

