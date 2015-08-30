#pragma once

#include <czmq.h>
#include <functional>
#include "ris/snapshot/snapshotbuilder.h"

#define SNAPSHOT_CLIENT_ERROR			-1
#define SNAPSHOT_CLIENT_TIMEOUT			-2

class SnapshotClient {
public:
	SnapshotClient(zloop_t* loop);
	~SnapshotClient();

	int start(const std::function<void(int)>& ob,const std::shared_ptr<ISnapshotBuilder>& builder,const std::string& address);
	void stop();
private:
	int pullSnapshotBegin(zsock_t* sock);
	int pullRegionOrFinish(zsock_t* sock);
	int pullRegionBody(zsock_t* sock);

private:
	static int readableAdapter(zloop_t* loop,zsock_t* reader,void* arg);
	static int timerAdapter(zloop_t* loop,int timer_id,void* arg);

	int onTimeoutTimer();
	int onReqReadable(zsock_t* sock);
private:
	zloop_t*								m_loop;
	zsock_t*								m_sock;
	int										m_tid;
	ri_time_t								m_tv_timeout;
	std::function<int(zsock_t*)>			m_fn_readable;

	std::function<void(int)>				m_observer;
	std::shared_ptr<ISnapshotBuilder>		m_builder;
	uuid_t									m_last_region;
};


