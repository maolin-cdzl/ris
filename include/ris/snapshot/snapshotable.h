#pragma once

#include "ris/snapshot/snapshot.h"

class ISnapshotable {
public:
	virtual std::shared_ptr<Snapshot> buildSnapshot() = 0;
};
