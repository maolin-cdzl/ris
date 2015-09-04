#include "mock_defines.h"
#include "test_helper.h"

#include "region/regiontable.h"

extern zloop_t* g_loop;

class RegionTableTest : public testing::Test {
protected:
	virtual void SetUp() {
		table = std::make_shared<RIRegionTable>(ctx,g_loop);
	}
	virtual void TearDown() {
		table.reset();
	}


	static void SetUpTestCase() {
		ctx = std::make_shared<RegionCtx>();
		ctx->uuid = "region-test";
		ctx->idc = "idc-test";
		ctx->api_address = "inproc://region-test";
		ctx->bus_address = "tcp://127.0.0.1:6600";
		ctx->pub_address = "tcp://127.0.0.1:2015";
		ctx->snapshot_svc_address = "tcp://127.0.0.1:6500";
		ctx->snapshot_worker_address = "tcp://127.0.0.1:![6501-6599]";
	}
	static void TearDownTestCase() {
		ctx.reset();
	}

	std::shared_ptr<RIRegionTable>			table;
	static std::shared_ptr<RegionCtx>		ctx;
};


std::shared_ptr<RegionCtx> RegionTableTest::ctx(nullptr);


TEST_F(RegionTableTest,Functional) {
	auto observer = std::make_shared<MockObserver>();

	EXPECT_CALL(*observer,onRegion(testing::_)).Times(1);
	EXPECT_CALL(*observer,onRmRegion(testing::_)).Times(1);
	EXPECT_CALL(*observer,onService(testing::_,testing::_,testing::_)).Times(3);
	EXPECT_CALL(*observer,onRmService(testing::_,testing::_,testing::_)).Times(1);
	EXPECT_CALL(*observer,onPayload(testing::_,testing::_,testing::_)).Times(3);
	EXPECT_CALL(*observer,onRmPayload(testing::_,testing::_,testing::_)).Times(1);

	ASSERT_EQ(0,table->start(observer));

	ASSERT_EQ(0,table->addService("service-1","tcp://127.0.0.1:*"));
	ASSERT_EQ(0,table->addService("service-2","tcp://127.0.0.1:*"));
	ASSERT_EQ(0,table->addService("service-3","tcp://127.0.0.1:*"));
	ASSERT_EQ(-1,table->addService("service-1","tcp://127.0.0.1:*"));

	ASSERT_EQ(0,table->addPayload("payload-1"));
	ASSERT_EQ(0,table->addPayload("payload-2"));
	ASSERT_EQ(0,table->addPayload("payload-3"));
	ASSERT_EQ(-1,table->addPayload("payload-1"));

	ASSERT_EQ(size_t(3),table->service_size());
	ASSERT_EQ(size_t(3),table->payload_size());

	ASSERT_EQ(0,table->rmService("service-3"));
	ASSERT_EQ(-1,table->rmService("service-UNKNOWN"));

	ASSERT_EQ(0,table->rmPayload("payload-3"));
	ASSERT_EQ(-1,table->rmPayload("payload-UNKNOWN"));

	ASSERT_EQ(uint32_t(8),table->version());

	auto package = table->buildSnapshot();
	ASSERT_EQ(size_t(8),package.size());

	table->stop();
}

