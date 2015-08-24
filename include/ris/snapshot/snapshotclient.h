#pragma once

#include "ris/loopable.h"
#include "ris/snapshot/snapshotbuilder.h"


class SnapshotClient : public ILoopable {
public:
	SnapshotClient(zloop_t* loop);

	virtual ~SnapshotClient();

	int start(const std::shared_ptr<ISnapshotBuilder>& builder,const std::string& serverAddress);
	int stop();
protected:
	virtual int startLoop(zloop_t* loop);
	virtual void stopLoop(zloop_t* loop);

private:
	zloop_t*								m_loop;
	zsock_t*								m_sock;
	std::shared_ptr<ISnapshotBuilder>		m_builder;
	std::string								m_address;
	std::shared_ptr<Snapshot>				m_snapshot;
};


