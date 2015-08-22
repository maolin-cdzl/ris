#pragma once

#include "ris/snapshot/snapshot.h"

class ISnapshotBuilder {
public:
	virtual int buildStart(std::shared_ptr<Snapshot>& snapshot) = 0;
	virtual int buildFinish(std::shared_ptr<Snapshot>& snapshot) = 0;
};

