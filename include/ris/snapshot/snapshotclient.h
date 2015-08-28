#pragma once

#include <czmq.h>
#include "ris/snapshot/snapshotbuilder.h"

class SnapshotClient {
public:
	SnapshotClient();

	virtual ~SnapshotClient();

	int requestSnapshot(const std::shared_ptr<ISnapshotBuilder>& builder,const std::string& address);

private:
	int pullSnapshotBegin(zsock_t* sock);
	int pullRegionOrFinish(zsock_t* sock);
	int pullRegionBody(zsock_t* sock);
	int pullSnapshot(zsock_t* sock);
private:
	std::shared_ptr<ISnapshotBuilder>		m_builder;
	uuid_t									m_last_region;
};


