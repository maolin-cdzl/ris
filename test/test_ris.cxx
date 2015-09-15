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
		tracker_inst = tracker_new_str(TRACKER_CONFIG);
		region_inst = region_new_str(REGION_CONFIG,0);
	}
	virtual void TearDown() {
		tracker_destroy(tracker_inst);
		region_destroy(region_inst);
	}

private:
	void* tracker_inst;
	void* region_inst;
};

static void region_add_services(const std::shared_ptr<RegionSession>& region,std::list<std::string>& services,size_t count,uint32_t* version=nullptr) {
	while( count > 1 ) {
		auto svc = newUUID();
		services.push_back(svc);
		ASSERT_EQ(0,region->asyncNewService(svc,"Unexists"));
		--count;
	}
	
	auto svc = newUUID();
	services.push_back(svc);
	ASSERT_EQ(0,region->newService(svc,"Unexists",version));
}

static void region_rm_services(const std::shared_ptr<RegionSession>& region,std::list<std::string>& services,size_t count,uint32_t* version=nullptr) {
	while( count > 1 && !services.empty()  ) {
		auto& svc = services.front();
		ASSERT_EQ(0,region->asyncRmService(svc));
		services.pop_front();
		--count;
	}

	if( ! services.empty() ) {
		auto& svc = services.front();
		ASSERT_EQ(0,region->rmService(svc,version));
		services.pop_front();
	}
}

static void region_add_payloads(const std::shared_ptr<RegionSession>& region,std::list<ri_uuid_t>& payloads,size_t count,uint32_t* version=nullptr) {
	while( count > 1 ) {
		auto pl = newUUID();
		payloads.push_back(pl);
		ASSERT_EQ(0,region->asyncNewPayload(pl));
		--count;
	}
	auto pl = newUUID();
	payloads.push_back(pl);
	ASSERT_EQ(0,region->newPayload(pl,version));
}

static void region_rm_payloads(const std::shared_ptr<RegionSession>& region,std::list<ri_uuid_t>& payloads,size_t count,uint32_t* version=nullptr) {
	while( count > 1 && !payloads.empty() ) {
		auto& pl = payloads.front();
		ASSERT_EQ(0,region->asyncRmPayload(pl));
		payloads.pop_front();
		--count;
	}
	if( ! payloads.empty() ) {
		auto& pl = payloads.front();
		ASSERT_EQ(0,region->rmPayload(pl,version));
		payloads.pop_front();
	}
}

static void get_tracker_region_version(const std::shared_ptr<TrackerSession>& tracker,const ri_uuid_t& reg,uint32_t* version) {
	RegionInfo reginfo;
	if( 1 == tracker->getRegion(&reginfo,reg)) {
		*version = reginfo.version;
	} else {
		*version = 0;
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
	auto region = std::make_shared<RegionSession>();
	ASSERT_EQ(0,region->connect("inproc://region-test-001"));

	std::list<std::string> services;
	std::list<std::string> payloads;

	region_add_services(region,services,100);
	region_add_payloads(region,payloads,50000);

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
	
	uint32_t tracker_reg_version = 0;
	get_tracker_region_version(tracker,"test-001",&tracker_reg_version);
	std::cout << "Tracker record region version: " << tracker_reg_version << std::endl;
}

TEST_F(RISTest,TrackSpeed) {
	auto region = std::make_shared<RegionSession>();
	ASSERT_EQ(0,region->connect("inproc://region-test-001"));

	std::list<std::string> services;
	std::list<std::string> payloads;

	uint32_t region_version = 0;
	region_add_services(region,services,100);
	region_add_payloads(region,payloads,50000,&region_version);

	auto tracker = std::make_shared<TrackerSession>();
	ASSERT_EQ(0,tracker->connect("inproc://tracker-test-001",500));

	const ri_time_t tv_start = ri_time_now();
	uint32_t tracker_reg_version = 0;
	while( 1 ) {
		get_tracker_region_version(tracker,"test-001",&tracker_reg_version);
		if( tracker_reg_version == region_version) {
			break;
		}
		usleep(10*1000);
	}

	std::cout << "Tracker used " << ri_time_now() - tv_start << " to track region" << std::endl;


	sleep(1);
	check_tracker_statistics(tracker,1,services.size(),payloads.size());
	check_tracker_region(tracker,"test-001",services,payloads);
}
