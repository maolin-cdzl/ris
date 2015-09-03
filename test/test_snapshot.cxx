#include "mock_defines.h"

#include "snapshot/snapshotservice.h"
#include "snapshot/snapshotserviceworker.h"
#include "snapshot/snapshotclient.h"
#include "snapshotgenerator.h"
#include "test_helper.h"


static void onSnapshotRequestDone(int* result,int err) {
	*result = err;
	zsys_interrupted = 1;
}

TEST(Snapshot,Normal) {
	static const char* SS_WORKER_ADDRESS = "tcp://127.0.0.1:3824";
	auto builder = std::make_shared<MockSnapshotBuilder>();

	EXPECT_CALL(*builder,addRegion(testing::_)).Times(testing::Between(1,5)).WillRepeatedly(testing::Return(0));
	EXPECT_CALL(*builder,addPayload(testing::_,testing::_)).Times(testing::Between(1,500)).WillRepeatedly(testing::Return(0));
	EXPECT_CALL(*builder,addService(testing::_,testing::_)).Times(testing::Between(1,100)).WillRepeatedly(testing::Return(0));

	zloop_t* loop = zloop_new();
	auto worker = std::make_shared<SnapshotServiceWorker>(SS_WORKER_ADDRESS);
	auto client = std::make_shared<SnapshotClient>(loop);

	int result;
	int done = -1;

	result = worker->start(SnapshotGenerator(5,100,20)());
	ASSERT_EQ(0,result);
	ASSERT_EQ(worker->endpoint(),SS_WORKER_ADDRESS);

	result = client->start(std::bind(onSnapshotRequestDone,&done,std::placeholders::_1),builder,SS_WORKER_ADDRESS);
	ASSERT_EQ(0,result);

	auto reader = std::make_shared<ReadableHelper>(loop);
	result = reader->register_read(zactor_sock(worker->actor()));
	ASSERT_EQ(0,result);

	result = zloop_start(loop);
	ASSERT_EQ(0,result);
	
	ASSERT_EQ(0,done);
	reader->unregister();
	zmsg_t* msg = reader->message();
	ASSERT_EQ(size_t(1),zmsg_size(msg));
	ASSERT_TRUE( zframe_streq(zmsg_first(msg),"ok") );

	zloop_destroy(&loop);
}

