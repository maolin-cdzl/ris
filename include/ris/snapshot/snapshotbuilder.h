#pragma once

#include "ris/snapshot/snapshot.h"

class ISnapshotBuilder {
public:
	virtual void buildError(int err,const std::string& what) = 0;
	virtual int build(std::shared_ptr<Snapshot>& snapshot) = 0;
};

