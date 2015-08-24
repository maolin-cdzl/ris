#pragma once

#include "ris/snapshot/snapshotbuilder.h"


class SnapshotClient {
public:
	SnapshotClient();

	virtual ~SnapshotClient();

	int requestSnapshot(const std::shared_ptr<ISnapshotBuilder>& builder,const std::string& address);

private:
	int pullSnapshotHeader(zsock_t* sock);
	int pullPartitionOrFinish(zsock_t* sock);
	int pullPartitionBody(zsock_t* sock);
	int pullSnapshot(zsock_t* sock);
private:
	std::shared_ptr<ISnapshotBuilder>		m_builder;
	std::shared_ptr<Snapshot>				m_snapshot;
	std::shared_ptr<SnapshotPartition>		m_part;
};


