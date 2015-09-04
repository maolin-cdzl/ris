#pragma once

#include <gmock/gmock.h>
#include "ris/riobserver.h"
#include "snapshot/snapshotable.h"
#include "snapshot/snapshotbuilder.h"

class MockObserver : public IRIObserver {
public:
	MOCK_METHOD1(onRegion,void(const Region&));
	MOCK_METHOD1(onRmRegion,void(const ri_uuid_t&));
	MOCK_METHOD3(onService,void(const ri_uuid_t&,uint32_t,const Service&));
	MOCK_METHOD3(onRmService,void(const ri_uuid_t&,uint32_t,const std::string&));
	MOCK_METHOD3(onPayload,void(const ri_uuid_t&,uint32_t,const Payload&));
	MOCK_METHOD3(onRmPayload,void(const ri_uuid_t&,uint32_t,const ri_uuid_t&));
};

class MockSnapshotable : public ISnapshotable {
public:
	MOCK_METHOD0(buildSnapshot,snapshot_package_t());
};

class MockSnapshotBuilder : public ISnapshotBuilder {
public:
	MOCK_METHOD1(addRegion,int(const Region&));
	MOCK_METHOD2(addService,int(const ri_uuid_t&,const Service& svc));
	MOCK_METHOD2(addPayload,int(const ri_uuid_t&,const Payload&));
};

