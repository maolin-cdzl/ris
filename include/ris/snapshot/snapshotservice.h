#pragma once

#include "ris/loopable.h"
#include "ris/snapshot/snapshotable.h"
#include "ris/snapshot/snapshotserviceworker.h"


class SnapshotService : public ILoopable {
public:
	SnapshotService(const std::shared_ptr<ISnapshotable>& snapshotable,const std::string& svcExpress,const std::string& workerExpress,size_t capacity=4);

	virtual ~SnapshotService();

	virtual int startLoop(zloop_t* loop);
	virtual void stopLoop(zloop_t* loop);

private:
	static int mainReaderAdapter(zloop_t* loop,zsock_t* reader,void* arg);
	static int workerReaderAdapter(zloop_t* loop,zsock_t* reader,void* arg);

	int onMainReadable(zloop_t* loop);
	int onWorkerReadable(zloop_t* loop,zsock_t* reader);
	zactor_t* createWorker();

	std::shared_ptr<SnapshotServiceWorker> findWorker(zsock_t* sock);
private:
	std::shared_ptr<ISnapshotable>	m_snapshotable;
	const std::string				m_svc_express;
	const std::string				m_worker_express;
	const size_t					m_capacity;

	zsock_t*						m_sock;
	std::list<std::shared_ptr<SnapshotServiceWorker>>			m_workers;
};

