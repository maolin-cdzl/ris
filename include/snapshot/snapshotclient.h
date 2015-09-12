#pragma once

#include <czmq.h>
#include <functional>
#include "snapshot/snapshotbuilder.h"
#include "zmqx/zloopreader.h"
#include "zmqx/zlooptimer.h"

class SnapshotClient {
public:
	SnapshotClient(zloop_t* loop);
	~SnapshotClient();

	int start(const std::shared_ptr<ISnapshotBuilder>& builder,const std::string& address);
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
	std::string								m_uuid;
	ZLoopTimeouter							m_timer;
	ZLoopReader								m_reader;

	std::shared_ptr<ISnapshotBuilder>		m_builder;
	ri_uuid_t								m_last_region;
};


