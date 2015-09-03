#pragma once

#include <gmock/gmock.h>
#include "snapshot/snapshotable.h"
#include "snapshot/snapshotbuilder.h"

class MockSnapshotable : public ISnapshotable {
public:
	MOCK_METHOD0(buildSnapshot,snapshot_package_t());
};

class MockSnapshotBuilder : public ISnapshotBuilder {
public:
	MOCK_METHOD1(addRegion,int(const Region&));
	MOCK_METHOD2(addService,int(const uuid_t&,const Service& svc));
	MOCK_METHOD2(addPayload,int(const uuid_t&,const Payload&));
};

