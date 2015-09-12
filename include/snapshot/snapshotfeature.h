#pragma once

#include <czmq.h>
#include "snapshot/snapshotable.h"
#include "zmqx/zloopreader.h"

class SnapshotFeature : public ISnapshotable {
public:
	SnapshotFeature(zloop_t* loop);
	virtual ~SnapshotFeature();

	/**
	 * !!! note !!!
	 * one SnapshotFeature can be called by one thread,call from multipli thread is UNDEFINED.
	 */
	virtual snapshot_package_t buildSnapshot();

	int start(const std::shared_ptr<ISnapshotable>& impl);
	void stop();
private:
	int onRequest(zsock_t* sock);
private:
	zloop_t*							m_loop;
	zsock_t*							m_frontend;
	zsock_t*							m_backend;
	ZLoopReader							m_backreader;
	std::shared_ptr<ISnapshotable>		m_impl;
	snapshot_package_t					m_package;
};

