#include "mock_defines.h"

#include "snapshot/snapshotservice.h"
#include "snapshot/snapshotserviceworker.h"
#include "snapshot/snapshotclient.h"
#include "snapshotgenerator.h"
#include "test_helper.h"

extern zloop_t* g_loop;
static const char* SS_SERVER_ADDRESS = "tcp://127.0.0.1:3824";
static const char* SS_WORKER_ADDRESS = "tcp://127.0.0.1:![3825-3850]";

template<typename RepeaterT>
void snapshot_testcase(size_t repeat_count) {
	auto snapshotable = std::make_shared<MockSnapshotable>();
	auto builder = std::make_shared<MockSnapshotBuilder>();
	SnapshotGenerator::reset_count();
	SnapshotGenerator generator(10,100,20);

	EXPECT_CALL(*snapshotable,buildSnapshot()).Times(repeat_count).WillRepeatedly(testing::Invoke(generator));
	EXPECT_CALL(*builder,addRegion(testing::_)).Times(RegionCardinality()).WillRepeatedly(testing::Return(0));
	EXPECT_CALL(*builder,addPayload(testing::_,testing::_)).Times(PayloadCardinality()).WillRepeatedly(testing::Return(0));
	EXPECT_CALL(*builder,addService(testing::_,testing::_)).Times(ServiceCardinality()).WillRepeatedly(testing::Return(0));

	auto server = std::make_shared<SnapshotService>(g_loop);
	auto repeater = std::make_shared<RepeaterT>(g_loop);

	int result;

	result = server->start(snapshotable,SS_SERVER_ADDRESS,SS_WORKER_ADDRESS);
	ASSERT_EQ(0,result);

	result = repeater->start(repeat_count,builder,SS_SERVER_ADDRESS);
	ASSERT_EQ(0,result);


	zsys_interrupted = 0;
	result = zloop_start(g_loop);
	zsys_interrupted = 0;
	ASSERT_EQ(0,result);

	ASSERT_EQ(repeat_count,repeater->success_count());
}



TEST(Snapshot,Normal) {
	snapshot_testcase<SnapshotClientRepeater>(1);
}

TEST(Snapshot,Repeat) {
	snapshot_testcase<SnapshotClientRepeater>(10);
}

TEST(Snapshot,Parallel) {
	snapshot_testcase<SnapshotClientParallelRepeater>(4);
}

TEST(Snapshot,ParallelOverflow) {
	const size_t repeat_count = 5;

	auto snapshotable = std::make_shared<MockSnapshotable>();
	auto builder = std::make_shared<MockSnapshotBuilder>();
	SnapshotGenerator::reset_count();
	SnapshotGenerator generator(10,100,20);

	EXPECT_CALL(*snapshotable,buildSnapshot()).Times(repeat_count).WillRepeatedly(testing::Invoke(generator));
	EXPECT_CALL(*builder,addRegion(testing::_)).Times(RegionCardinality()).WillRepeatedly(testing::Return(0));
	EXPECT_CALL(*builder,addPayload(testing::_,testing::_)).Times(PayloadCardinality()).WillRepeatedly(testing::Return(0));
	EXPECT_CALL(*builder,addService(testing::_,testing::_)).Times(ServiceCardinality()).WillRepeatedly(testing::Return(0));

	auto server = std::make_shared<SnapshotService>(g_loop);
	auto repeater = std::make_shared<SnapshotClientParallelRepeater>(g_loop);

	int result;

	result = server->start(snapshotable,SS_SERVER_ADDRESS,SS_WORKER_ADDRESS);
	ASSERT_EQ(0,result);

	result = repeater->start(repeat_count,builder,SS_SERVER_ADDRESS);
	ASSERT_EQ(0,result);


	zsys_interrupted = 0;
	result = zloop_start(g_loop);
	zsys_interrupted = 0;
	ASSERT_EQ(0,result);

	ASSERT_EQ(repeat_count,repeater->success_count());
}
