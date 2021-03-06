#pragma once

#include <czmq.h>
#include <functional>
#include "snapshot/snapshotbuilder.h"
#include "zmqx/zloopreader.h"
#include "zmqx/zlooptimer.h"

class SnapshotClientWorker {
public:
	SnapshotClientWorker(zloop_t* loop);
	~SnapshotClientWorker();

	int start(const std::function<void(int)>& completed,const std::shared_ptr<ISnapshotBuilder>& builder,const std::string& address);
	void stop();

	bool isActive() const;

	std::string state() const;
private:
	int onReqReadable(zsock_t* sock);
	int pullSnapshotBegin(zsock_t* sock);
	int pullRegionBegin(zsock_t* sock);
	int pullRegionOrFinish(zsock_t* sock);
	int pullRegionBody(zsock_t* sock);
	int onTimeoutTimer();

private:
	void finish(int err);
private:
	zloop_t*								m_loop;
	std::string								m_requester;
	ZLoopTimeouter							m_timer;
	ZLoopReader								m_reader;

	std::function<void(int)>				m_completed;
	std::shared_ptr<ISnapshotBuilder>		m_builder;
	ri_uuid_t								m_last_region;
};


