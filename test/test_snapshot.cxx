#include "mock_defines.h"

#include "snapshot/snapshotservice.h"
#include "snapshot/snapshotserviceworker.h"
#include "snapshot/snapshotclient.h"
#include "snapshotgenerator.h"
#include "test_helper.h"

extern zloop_t* g_loop;
static const char* SS_SERVER_ADDRESS = "tcp://127.0.0.1:3824";
static const char* SS_WORKER_ADDRESS = "tcp://127.0.0.1:![3825-3850]";

TEST(Snapshot,Normal) {
	auto snapshotable = std::make_shared<MockSnapshotable>();
	auto builder = std::make_shared<MockSnapshotBuilder>();

	SnapshotGenerator::reset_count();
	SnapshotGenerator generator(10,100,20);
	
	EXPECT_CALL(*snapshotable,buildSnapshot()).Times(1).WillRepeatedly(testing::Invoke(generator));
	EXPECT_CALL(*builder,addRegion(testing::_)).Times(RegionCardinality()).WillRepeatedly(testing::Return(0));
	EXPECT_CALL(*builder,addPayload(testing::_,testing::_)).Times(PayloadCardinality()).WillRepeatedly(testing::Return(0));
	EXPECT_CALL(*builder,addService(testing::_,testing::_)).Times(ServiceCardinality()).WillRepeatedly(testing::Return(0));


	auto server = std::make_shared<SnapshotService>(g_loop);
	auto client = std::make_shared<SnapshotClient>(g_loop);

	int result;
	CompleteResultHelper compleresult;

	result = server->start(snapshotable,SS_SERVER_ADDRESS,SS_WORKER_ADDRESS);
	ASSERT_EQ(0,result);

	result = client->start(std::bind(&CompleteResultHelper::onComplete,&compleresult,std::placeholders::_1),builder,SS_SERVER_ADDRESS);
	ASSERT_EQ(0,result);

	zsys_interrupted = 0;
	result = zloop_start(g_loop);
	zsys_interrupted = 0;
	ASSERT_EQ(0,result);
	ASSERT_EQ(0,compleresult.result());
}

TEST(Snapshot,Repeat) {
	static const size_t REPEAT_COUNT = 10;

	auto snapshotable = std::make_shared<MockSnapshotable>();
	auto builder = std::make_shared<MockSnapshotBuilder>();
	SnapshotGenerator::reset_count();
	SnapshotGenerator generator(10,100,20);

	EXPECT_CALL(*snapshotable,buildSnapshot()).Times(REPEAT_COUNT).WillRepeatedly(testing::Invoke(generator));
	EXPECT_CALL(*builder,addRegion(testing::_)).Times(RegionCardinality()).WillRepeatedly(testing::Return(0));
	EXPECT_CALL(*builder,addPayload(testing::_,testing::_)).Times(PayloadCardinality()).WillRepeatedly(testing::Return(0));
	EXPECT_CALL(*builder,addService(testing::_,testing::_)).Times(ServiceCardinality()).WillRepeatedly(testing::Return(0));

	auto server = std::make_shared<SnapshotService>(g_loop);
	auto repeater = std::make_shared<SnapshotClientRepeater>(g_loop);

	int result;

	result = server->start(snapshotable,SS_SERVER_ADDRESS,SS_WORKER_ADDRESS);
	ASSERT_EQ(0,result);

	result = repeater->start(REPEAT_COUNT,builder,SS_SERVER_ADDRESS);
	ASSERT_EQ(0,result);


	zsys_interrupted = 0;
	result = zloop_start(g_loop);
	zsys_interrupted = 0;
	ASSERT_EQ(0,result);
}

TEST(Snapshot,Parallel) {
	static const size_t REPEAT_COUNT = 4;

	auto snapshotable = std::make_shared<MockSnapshotable>();
	auto builder = std::make_shared<MockSnapshotBuilder>();
	SnapshotGenerator::reset_count();
	SnapshotGenerator generator(10,100,20);

	EXPECT_CALL(*snapshotable,buildSnapshot()).Times(REPEAT_COUNT).WillRepeatedly(testing::Invoke(generator));
	EXPECT_CALL(*builder,addRegion(testing::_)).Times(RegionCardinality()).WillRepeatedly(testing::Return(0));
	EXPECT_CALL(*builder,addPayload(testing::_,testing::_)).Times(PayloadCardinality()).WillRepeatedly(testing::Return(0));
	EXPECT_CALL(*builder,addService(testing::_,testing::_)).Times(ServiceCardinality()).WillRepeatedly(testing::Return(0));

	auto server = std::make_shared<SnapshotService>(g_loop);
	auto repeater = std::make_shared<SnapshotClientParallelRepeater>(g_loop);

	int result;

	result = server->start(snapshotable,SS_SERVER_ADDRESS,SS_WORKER_ADDRESS);
	ASSERT_EQ(0,result);

	result = repeater->start(REPEAT_COUNT,builder,SS_SERVER_ADDRESS);
	ASSERT_EQ(0,result);


	zsys_interrupted = 0;
	result = zloop_start(g_loop);
	zsys_interrupted = 0;
	ASSERT_EQ(0,result);
}

