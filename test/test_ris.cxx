#include "mock_defines.h"

#include "test_helper.h"
#include "region/regionapi.h"
#include "tracker/trackerapi.h"
#include "trackersession.h"
#include "regionsession.h"

extern zloop_t* g_loop;

static const char* REGION_CONFIG =
"region:\n"
"{\n"
"	id = \"test-001\";\n"
"	idc = \"test-idc\";\n"
"	api_address = \"inproc://region-test-001\";\n"
"	bus_address = \"tcp://127.0.0.1:6600\";\n"
"	pub_address = \"tcp://127.0.0.1:2015\";\n"
"	bind_pub=True\n"
"\n"
"	snapshot:\n"
"	{\n"
"		address = \"tcp://127.0.0.1:6500\";\n"
"		worker_address = \"tcp://127.0.0.1:![6501-6599]\";\n"
"	};\n"
"\n"
"};\n";


static const char* TRACKER_CONFIG =
"tracker:\n"
"{\n"
"	idc = \"test-idc\";\n"
"	api_address = \"inproc://tracker-test-001\";\n"
"	pub_address = \"tcp://127.0.0.1:2015\";\n"
"	factory_timeout = 6000;\n"
"	snapshot:\n"
"	{\n"
"		address = \"tcp://127.0.0.1:7500\";\n"
"		worker_address = \"tcp://127.0.0.1:![7501-7599]\";\n"
"	};\n"
"\n"
"};\n";



class RISTest : public testing::Test {
protected:
	virtual void SetUp() {
	}
	virtual void TearDown() {
		tracker_stop();
		region_stop();
	}

	static void SetUpTestCase() {
		for(size_t i=0; i < 10; ++i) {
			auto svc = newUUID();
			services.push_back(svc);
		}

		for(size_t i=0; i < 100; ++i) {
			auto pl = newUUID();
			payloads.push_back(pl);
		}
	}

	static void TearDownTestCase() {
	}

	static std::list<std::string> services;
	static std::list<std::string> payloads;
};

std::list<std::string> RISTest::services;
std::list<std::string> RISTest::payloads;

TEST_F(RISTest,Functional) {
	ASSERT_EQ(0,tracker_start_str(TRACKER_CONFIG));
	ASSERT_EQ(0,region_start_str(REGION_CONFIG,0));

	sleep(10);

	auto tracker = std::make_shared<TrackerSession>();
	ASSERT_EQ(0,tracker->connect("inproc://tracker-test-001",500));

	auto region = std::make_shared<RegionSession>();
	ASSERT_EQ(0,region->connect("inproc://region-test-001",500));


	for(auto it=services.begin(); it != services.end(); ++it) {
		region->newService(*it,"Unexists");
	}

	for(auto it=payloads.begin(); it != payloads.end(); ++it) {
		region->newPayload(*it);
	}

	sleep(1);

	RouteInfoStatistics stat;
	ASSERT_EQ(1,tracker->getStatistics(&stat));
	ASSERT_EQ(size_t(1),stat.region_size);
	ASSERT_EQ(services.size(),stat.service_size);
	ASSERT_EQ(payloads.size(),stat.payload_size);
}


