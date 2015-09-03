#include "mock_defines.h"

#include "snapshot/snapshotservice.h"
#include "snapshot/snapshotserviceworker.h"
#include "snapshot/snapshotclient.h"
#include "snapshotgenerator.h"
#include "test_helper.h"

extern zloop_t* g_loop;

static void onSnapshotRequestDone(int* result,int err) {
	*result = err;
	zsys_interrupted = 1;
}

TEST(Snapshot,Normal) {
	static const char* SS_SERVER_ADDRESS = "tcp://127.0.0.1:3824";
	static const char* SS_WORKER_ADDRESS = "tcp://127.0.0.1:3825";
	auto snapshotable = std::make_shared<MockSnapshotable>();
	auto builder = std::make_shared<MockSnapshotBuilder>();
	SnapshotGenerator generator(10,100,20);
	auto snapshot = generator();
	
	EXPECT_CALL(*snapshotable,buildSnapshot()).Times(1).WillRepeatedly(testing::Return(snapshot));
	EXPECT_CALL(*builder,addRegion(testing::_)).Times(generator.region_size()).WillRepeatedly(testing::Return(0));
	EXPECT_CALL(*builder,addPayload(testing::_,testing::_)).Times(generator.payload_size()).WillRepeatedly(testing::Return(0));
	EXPECT_CALL(*builder,addService(testing::_,testing::_)).Times(generator.service_size()).WillRepeatedly(testing::Return(0));

	auto server = std::make_shared<SnapshotService>(g_loop);
	auto client = std::make_shared<SnapshotClient>(g_loop);

	int result;
	int done = -1;

	result = server->start(snapshotable,SS_SERVER_ADDRESS,SS_WORKER_ADDRESS);
	ASSERT_EQ(0,result);

	result = client->start(std::bind(onSnapshotRequestDone,&done,std::placeholders::_1),builder,SS_SERVER_ADDRESS);
	ASSERT_EQ(0,result);


	result = zloop_start(g_loop);
	ASSERT_EQ(0,result);
	
	ASSERT_EQ(0,done);
}

