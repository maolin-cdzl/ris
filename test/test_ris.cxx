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
"	factory_timeout = 800;\n"
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
};

static void region_push_services(const std::shared_ptr<RegionSession>& region,std::list<std::string>& services,size_t count) {
	for(size_t i=0; i < count; ++i) {
		auto svc = newUUID();
		services.push_back(svc);
		ASSERT_EQ(0,region->newService(svc,"Unexists"));
	}
}

static void region_push_payloads(const std::shared_ptr<RegionSession>& region,std::list<ri_uuid_t>& payloads,size_t count) {
	for(size_t i=0; i < count; ++i) {
		auto pl = newUUID();
		payloads.push_back(pl);
		ASSERT_EQ(0,region->newPayload(pl));
	}
}

static void check_tracker_statistics(const std::shared_ptr<TrackerSession>& tracker,size_t region_count,size_t service_count,size_t payload_count) {
	RouteInfoStatistics stat;
	ASSERT_EQ(1,tracker->getStatistics(&stat));
	ASSERT_EQ(region_count,stat.region_size);
	ASSERT_EQ(service_count,stat.service_size);
	ASSERT_EQ(payload_count,stat.payload_size);
}

static void check_tracker_region(const std::shared_ptr<TrackerSession>& tracker,const ri_uuid_t& reg,const std::list<std::string>& services,const std::list<std::string>& payloads) {
	RegionInfo reginfo;
	ASSERT_EQ(1,tracker->getRegion(&reginfo,reg));

	for(auto it=services.begin(); it != services.end(); ++it) {
		RouteInfo ri;
		ASSERT_EQ(1,tracker->getServiceRouteInfo(&ri,*it));
		ASSERT_EQ(reginfo.uuid,ri.region);
	}

	for(auto it=payloads.begin(); it != payloads.end(); ++it) {
		RouteInfo ri;
		ASSERT_EQ(1,tracker->getPayloadRouteInfo(&ri,*it));
		ASSERT_EQ(reginfo.uuid,ri.region);
	}
}

TEST_F(RISTest,Functional) {
	ASSERT_EQ(0,region_start_str(REGION_CONFIG,0));
	ASSERT_EQ(0,tracker_start_str(TRACKER_CONFIG));

	auto region = std::make_shared<RegionSession>();
	ASSERT_EQ(0,region->connect("inproc://region-test-001",500));

	std::list<std::string> services;
	std::list<std::string> payloads;

	region_push_services(region,services,10);
	region_push_payloads(region,payloads,100);

	sleep(10);

	auto tracker = std::make_shared<TrackerSession>();
	ASSERT_EQ(0,tracker->connect("inproc://tracker-test-001",500));

	check_tracker_statistics(tracker,1,services.size(),payloads.size());
	check_tracker_region(tracker,"test-001",services,payloads);
}


