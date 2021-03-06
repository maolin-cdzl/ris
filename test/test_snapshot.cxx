#include "mock_defines.h"

#include "snapshot/snapshotservice.h"
#include "snapshot/snapshotserviceworker.h"
#include "snapshot/snapshotclient.h"
#include "snapshotgenerator.h"
#include "test_helper.h"

extern zloop_t* g_loop;
static const char* SS_SERVER_ADDRESS = "tcp://127.0.0.1:3824";

template<typename RepeaterT>
void snapshot_testcase(size_t repeat_count) {
	auto snapshotable = std::make_shared<MockSnapshotable>();
	auto builder = std::make_shared<MockSnapshotBuilder>();
	auto server = std::make_shared<SnapshotService>();
	auto repeater = std::make_shared<RepeaterT>(g_loop);

	auto generator = std::make_shared<SnapshotGenerator>(10,1000,20);
	std::function<snapshot_package_t()> func = std::bind<snapshot_package_t>(&ISnapshotGeneratorImpl::build,generator);

	EXPECT_CALL(*snapshotable,buildSnapshot()).Times(repeat_count).WillRepeatedly(testing::Invoke(func));
	EXPECT_CALL(*builder,addRegion(testing::_)).Times(RegionNumber(generator)).WillRepeatedly(testing::Return(0));
	EXPECT_CALL(*builder,addPayload(testing::_,testing::_)).Times(PayloadNumber(generator)).WillRepeatedly(testing::Return(0));
	EXPECT_CALL(*builder,addService(testing::_,testing::_)).Times(ServiceNumber(generator)).WillRepeatedly(testing::Return(0));

	auto sctx = std::make_shared<SnapshotCtx>(SS_SERVER_ADDRESS,"snapshotsvc");
	ASSERT_EQ(0,server->start(snapshotable,sctx));
	ASSERT_EQ(0,repeater->start(repeat_count,builder,SS_SERVER_ADDRESS));


	while( repeater->running() ) {
		if( 0 == zloop_start(g_loop) ) {
			break;
		}
	};

	ASSERT_EQ(repeat_count,repeater->success_count());
	//std::cerr << "region_size=" << generator->region_size() << ", payload_size=" << generator->payload_size() << ", service_size=" << generator->service_size() << std::endl;
}

template<typename RepeaterT>
void snapshot_partfail_testcase(size_t repeat_count) {
	auto snapshotable = std::make_shared<MockSnapshotable>();
	auto builder = std::make_shared<MockSnapshotBuilder>();
	auto server = std::make_shared<SnapshotService>();
	auto repeater = std::make_shared<RepeaterT>(g_loop);

	auto generator = std::make_shared<SnapshotGenerator>(10,1000,20);
	std::function<snapshot_package_t()> func = std::bind<snapshot_package_t>(&ISnapshotGeneratorImpl::build,generator);

	EXPECT_CALL(*snapshotable,buildSnapshot()).Times(testing::AtMost(repeat_count)).WillRepeatedly(testing::Invoke(func));
	EXPECT_CALL(*builder,addRegion(testing::_)).Times(RegionAtMost(generator)).WillRepeatedly(testing::Return(0));
	EXPECT_CALL(*builder,addPayload(testing::_,testing::_)).Times(PayloadAtMost(generator)).WillRepeatedly(testing::Return(0));
	EXPECT_CALL(*builder,addService(testing::_,testing::_)).Times(ServiceAtMost(generator)).WillRepeatedly(testing::Return(0));

	int result;

	auto sctx = std::make_shared<SnapshotCtx>(SS_SERVER_ADDRESS,"snapshotsvc");
	result = server->start(snapshotable,sctx);
	ASSERT_EQ(0,result);

	result = repeater->start(repeat_count,builder,SS_SERVER_ADDRESS);
	ASSERT_EQ(0,result);


	while( repeater->running() ) {
		if( 0 == zloop_start(g_loop) ) {
			break;
		}
	};

	ASSERT_GT(repeater->success_count(),size_t(0));
	ASSERT_LT(repeater->success_count(),repeat_count);
}

template<typename GeneratorT>
void snapshot_fail_testcase() {
	auto snapshotable = std::make_shared<MockSnapshotable>();
	auto builder = std::make_shared<MockSnapshotBuilder>();
	auto server = std::make_shared<SnapshotService>();
	auto repeater = std::make_shared<SnapshotClientRepeater>(g_loop);

	auto generator = std::make_shared<GeneratorT>();
	std::function<snapshot_package_t()> func = std::bind<snapshot_package_t>(&ISnapshotGeneratorImpl::build,generator);


	EXPECT_CALL(*snapshotable,buildSnapshot()).Times(1).WillRepeatedly(testing::Invoke(func));
	EXPECT_CALL(*builder,addRegion(testing::_)).Times(RegionNumber(generator)).WillRepeatedly(testing::Return(0));
	EXPECT_CALL(*builder,addPayload(testing::_,testing::_)).Times(PayloadNumber(generator)).WillRepeatedly(testing::Return(0));
	EXPECT_CALL(*builder,addService(testing::_,testing::_)).Times(ServiceNumber(generator)).WillRepeatedly(testing::Return(0));

	auto sctx = std::make_shared<SnapshotCtx>(SS_SERVER_ADDRESS,"snapshotsvc");
	ASSERT_EQ(0,server->start(snapshotable,sctx));
	ASSERT_EQ(0,repeater->start(1,builder,SS_SERVER_ADDRESS));

	while( repeater->running() ) {
		if( 0 == zloop_start(g_loop) ) {
			break;
		}
	};

	ASSERT_EQ(size_t(0),repeater->success_count());
}


TEST(Snapshot,Functional) {
	SCOPED_TRACE("Functional");
	snapshot_testcase<SnapshotClientRepeater>(1);
}

TEST(Snapshot,Repeat) {
	SCOPED_TRACE("Repeat");
	snapshot_testcase<SnapshotClientRepeater>(10);
}

TEST(Snapshot,Parallel) {
	SCOPED_TRACE("Parallel");
	snapshot_testcase<SnapshotClientParallelRepeater>(4);
}

TEST(Snapshot,ParallelOverflow) {
	SCOPED_TRACE("ParallelOverflow");
	snapshot_partfail_testcase<SnapshotClientParallelRepeater>(10);
}

TEST(Snapshot,Empty) {
	SCOPED_TRACE("Empty");
	snapshot_fail_testcase<EmptySnapshotGenerator>();
}

TEST(Snapshot,UnmatchedRegion) {
	SCOPED_TRACE("UnmatchedRegion");
	snapshot_fail_testcase<UnmatchedRegionGenerator>();
}

TEST(Snapshot,UncompletedRegion) {
	SCOPED_TRACE("UncompletedRegion");
	snapshot_fail_testcase<UncompletedRegionGenerator>();
}



