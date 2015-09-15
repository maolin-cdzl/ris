#pragma once


#include <czmq.h>
#include <functional>
#include "snapshot/snapshotbuilder.h"
#include "zmqx/zloopreader.h"


class SnapshotClient {
public:
	SnapshotClient(zloop_t* loop);
	~SnapshotClient();

	int start(const std::function<void(int)>& completed,const std::shared_ptr<ISnapshotBuilder>& builder,const std::string& address);
	void stop();

private:
	int onCallerPipeReadable(zsock_t* sock);
	int onRunnerPipeReadable(bool* running,zsock_t* sock);
	void onWorkerCompleted(zsock_t* pipe,int err);
	void run(zsock_t* pipe);
	static void actorRunner(zsock_t* pipe,void* arg);

private:
	zloop_t*								m_caller_loop;

	zactor_t*								m_actor;
	std::function<void(int)>				m_completed;
	std::shared_ptr<ISnapshotBuilder>		m_builder;
	std::string								m_address;
	std::shared_ptr<ZLoopReader>			m_actor_reader;
};

