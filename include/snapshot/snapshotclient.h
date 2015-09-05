#pragma once

#include <czmq.h>
#include <functional>
#include "snapshot/snapshotbuilder.h"
#include "zmqx/zloopreader.h"
#include "zmqx/zlooptimer.h"

#define SNAPSHOT_CLIENT_ERROR			-1
#define SNAPSHOT_CLIENT_TIMEOUT			-2

class SnapshotClient {
public:
	SnapshotClient(zloop_t* loop);
	~SnapshotClient();

	int start(const std::function<void(int)>& ob,const std::shared_ptr<ISnapshotBuilder>& builder,const std::string& address);
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
	void cancelRegion();
private:
	zloop_t*								m_loop;
	ZLoopTimeouter							m_timer;
	ZLoopReader								m_reader;

	std::function<void(int)>				m_observer;
	std::shared_ptr<ISnapshotBuilder>		m_builder;
	ri_uuid_t								m_last_region;
};


