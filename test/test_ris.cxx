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
"	pub_address = \"tcp://127.0.0.1:2016\";\n"
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

static void region_add_services(const std::shared_ptr<RegionSession>& region,std::list<std::string>& services,size_t count) {
	for(size_t i=0; i < count; ++i) {
		auto svc = newUUID();
		services.push_back(svc);
		ASSERT_EQ(0,region->newService(svc,"Unexists"));
	}
}

static void region_rm_services(const std::shared_ptr<RegionSession>& region,std::list<std::string>& services,size_t count) {
	size_t i = 0;
	while( i < count && !services.empty() ) {
		auto& svc = services.front();
		ASSERT_EQ(0,region->rmService(svc));
		services.pop_front();
		++i;
	}
}

static void region_add_payloads(const std::shared_ptr<RegionSession>& region,std::list<ri_uuid_t>& payloads,size_t count) {
	for(size_t i=0; i < count; ++i) {
		auto pl = newUUID();
		payloads.push_back(pl);
		ASSERT_EQ(0,region->newPayload(pl));
	}
}

static void region_rm_payloads(const std::shared_ptr<RegionSession>& region,std::list<ri_uuid_t>& payloads,size_t count) {
	size_t i = 0;
	while( i < count && !payloads.empty() ) {
		auto& pl = payloads.front();
		ASSERT_EQ(0,region->rmPayload(pl));
		payloads.pop_front();
		++i;
	}
}

static void check_tracker_statistics(const std::shared_ptr<TrackerSession>& tracker,size_t region_count,size_t service_count,size_t payload_count) {
	RouteInfoStatistics stat;
	ASSERT_EQ(1,tracker->getStatistics(&stat));
	ASSERT_EQ(region_count,stat.region_size);
	ASSERT_EQ(service_count,stat.service_size);
	ASSERT_EQ(payload_count,stat.payload_size);

	std::cout << "Checked tracker with " << region_count << " regions, " << service_count << " services, " << payload_count << " payloads" << std::endl;
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

	std::cout << "Checked region " << reg << " with " << services.size() << " services, " << payloads.size() << " payloads" << std::endl;
}

TEST_F(RISTest,Functional) {
	ASSERT_EQ(0,region_start_str(REGION_CONFIG,0));
	ASSERT_EQ(0,tracker_start_str(TRACKER_CONFIG));

	auto region = std::make_shared<RegionSession>();
	ASSERT_EQ(0,region->connect("inproc://region-test-001"));

	std::list<std::string> services;
	std::list<std::string> payloads;

	region_add_services(region,services,10);
	region_add_payloads(region,payloads,100);

	sleep(10);

	auto tracker = std::make_shared<TrackerSession>();
	ASSERT_EQ(0,tracker->connect("inproc://tracker-test-001",500));

	check_tracker_statistics(tracker,1,services.size(),payloads.size());
	check_tracker_region(tracker,"test-001",services,payloads);

	region_add_payloads(region,payloads,100);
	sleep(1);
	check_tracker_statistics(tracker,1,services.size(),payloads.size());
	check_tracker_region(tracker,"test-001",services,payloads);

	region_rm_services(region,services,5);
	region_rm_payloads(region,payloads,50);
	region_add_services(region,services,10);
	region_add_payloads(region,payloads,100);
	sleep(1);
	check_tracker_statistics(tracker,1,services.size(),payloads.size());
	check_tracker_region(tracker,"test-001",services,payloads);
	
}

TEST_F(RISTest,BusyRegion) {
	ASSERT_EQ(0,region_start_str(REGION_CONFIG,0));
	auto region = std::make_shared<RegionSession>();
	ASSERT_EQ(0,region->connect("inproc://region-test-001"));

	std::list<std::string> services;
	std::list<std::string> payloads;

	region_add_services(region,services,100);
	region_add_payloads(region,payloads,50000);

	ASSERT_EQ(0,tracker_start_str(TRACKER_CONFIG));

	const ri_time_t tv_end = ri_time_now() + 10000;
	while( ri_time_now() < tv_end ) {
		usleep(10);

		region_rm_services(region,services,9);
		region_add_services(region,services,10);
		region_rm_payloads(region,payloads,90);
		region_add_payloads(region,payloads,100);
	}

	auto tracker = std::make_shared<TrackerSession>();
	ASSERT_EQ(0,tracker->connect("inproc://tracker-test-001",500));

	sleep(1);
	check_tracker_statistics(tracker,1,services.size(),payloads.size());
	check_tracker_region(tracker,"test-001",services,payloads);
}

