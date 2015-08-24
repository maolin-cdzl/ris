#pragma once

#include "ris/snapshot/snapshot.h"

class ISnapshotBuilder {
public:
	virtual int build(std::shared_ptr<Snapshot>& snapshot) = 0;
};

