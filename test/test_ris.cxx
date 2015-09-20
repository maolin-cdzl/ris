#include <glog/logging.h>
#include "mock_defines.h"

#include "test_helper.h"
#include "region/regionapi.h"
#include "tracker/trackerapi.h"
#include "trackercli/trackersession.h"
#include "regioncli/regionsession.h"

extern zloop_t* g_loop;

struct RegionInstance {
	std::string				name;
	std::string				api_address;
	std::string				snapshot_address;
	void*					instance;

	RegionInstance() :
		instance(nullptr)
	{
	}
};

struct TrackerInstance {
	std::string				api_address;
	std::string				snapshot_address;
	void*					instance;

	TrackerInstance() :
		instance(nullptr)
	{
	}
};

class RISTest : public testing::Test {
protected:
	virtual void SetUp() {
		m_region_index = 1;
		m_tracker_index = 1;
		m_snapshot_port = 6500;

	}
	virtual void TearDown() {
		for(auto it = m_trackers.begin(); it != m_trackers.end(); ++it) {
			tracker_destroy(it->instance);
		}
		m_trackers.clear();

		for(auto it=m_regions.begin(); it != m_regions.end(); ++it) {
			region_destroy(it->instance);
		}
		m_regions.clear();
	}

	const RegionInstance& getRegion(size_t pos) const {
		CHECK_LT(pos,m_regions.size());
		return m_regions[pos];
	}

	const TrackerInstance& getTracker(size_t pos) const {
		CHECK_LT(pos,m_trackers.size());
		return m_trackers[pos];
	}

	void create_region() {
		RegionInstance region;
		std::stringstream ss;
		ss << "region-" << m_region_index++;
		region.name = ss.str();
		ss.str("");
		ss.clear();

		ss << "inproc://" << region.name;
		region.api_address = ss.str();
		ss.str("");
		ss.clear();

		ss << "tcp://127.0.0.1:" << m_snapshot_port++;
		region.snapshot_address = ss.str();
		ss.str("");
		ss.clear();

		ss <<
			"region:\n" <<
			"	{\n" <<
			"	id = \"" << region.name << "\";\n" <<
			"	idc = \"test-idc\";\n" <<
			"	api_address = \"" << region.api_address << "\";\n" <<
			"	pub_address = \"tcp://127.0.0.1:2015\";\n" <<
			"	bus_address = \"tcp://127.0.0.1:8888\";\n" <<
			"\n" <<
			"	snapshot:\n" <<
			"	{\n" <<
			"		address = \"" << region.snapshot_address << "\";\n" <<
			"	};\n" <<
			"\n" <<
			"};\n";

		std::string conf = ss.str();
		region.instance = region_new_str(conf.c_str(),0);

		m_regions.push_back(region);
	}

	void destroy_last_region() {
		auto it = std::prev(m_regions.end());
		RegionInstance region = *it;
		m_regions.erase(it);
	
		region_destroy(region.instance);
	}

	void create_tracker() {
		TrackerInstance tracker;
		std::stringstream ss;
		ss << "tracker-" << m_tracker_index++;
		auto name = ss.str();
		ss.str("");
		ss.clear();

		ss << "inproc://" << name;
		tracker.api_address = ss.str();
		ss.str("");
		ss.clear();

		ss << "tcp://127.0.0.1:" << m_snapshot_port++;
		tracker.snapshot_address = ss.str();
		ss.str("");
		ss.clear();

		ss <<
			"tracker:\n" <<
			"	{\n" <<
			"	idc = \"test-idc\";\n" <<
			"	api_address = \"" << tracker.api_address << "\";\n" <<
			"	pub_address = \"tcp://127.0.0.1:2016\";\n" <<
			"\n" <<
			"	snapshot:\n" <<
			"	{\n" <<
			"		address = \"" << tracker.snapshot_address << "\";\n" <<
			"	};\n" <<
			"\n" <<
			"};\n";

		std::string conf = ss.str();
		tracker.instance = tracker_new_str(conf.c_str());
		m_trackers.push_back(tracker);
	}

private:
	int						m_region_index;
	int						m_tracker_index;
	int						m_snapshot_port;
	std::vector<TrackerInstance>			m_trackers;
	std::vector<RegionInstance>			m_regions;
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

//	std::cout << "Checked tracker with " << region_count << " regions, " << service_count << " services, " << payload_count << " payloads" << std::endl;
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

//	std::cout << "Checked region " << reg << " with " << services.size() << " services, " << payloads.size() << " payloads" << std::endl;
}

TEST_F(RISTest,Functional) {
	create_tracker();
	create_region();
	auto region = std::make_shared<RegionSession>();
	ASSERT_EQ(0,region->connect(getRegion(0).api_address));

	std::list<std::string> services;
	std::list<std::string> payloads;

	region_add_services(region,services,10);
	region_add_payloads(region,payloads,100);

	sleep(10);

	auto tracker = std::make_shared<TrackerSession>();
	ASSERT_EQ(0,tracker->connect(getTracker(0).api_address,500));

	check_tracker_statistics(tracker,1,services.size(),payloads.size());
	check_tracker_region(tracker,getRegion(0).name,services,payloads);

	region_add_payloads(region,payloads,100);
	sleep(1);
	check_tracker_statistics(tracker,1,services.size(),payloads.size());
	check_tracker_region(tracker,getRegion(0).name,services,payloads);

	region_rm_services(region,services,5);
	region_rm_payloads(region,payloads,50);
	region_add_services(region,services,10);
	region_add_payloads(region,payloads,100);
	sleep(1);
	check_tracker_statistics(tracker,1,services.size(),payloads.size());
	check_tracker_region(tracker,getRegion(0).name,services,payloads);
	
}

TEST_F(RISTest,BusyRegion) {
	create_tracker();
	create_region();
	auto region = std::make_shared<RegionSession>();
	ASSERT_EQ(0,region->connect(getRegion(0).api_address));

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
	ASSERT_EQ(0,tracker->connect(getTracker(0).api_address,500));

	sleep(1);
	check_tracker_statistics(tracker,1,services.size(),payloads.size());
	check_tracker_region(tracker,getRegion(0).name,services,payloads);
	
	uint32_t tracker_reg_version = 0;
	get_tracker_region_version(tracker,getRegion(0).name,&tracker_reg_version);
//	std::cout << "Tracker record region version: " << tracker_reg_version << std::endl;
}

TEST_F(RISTest,MultiRegion) {
	static const size_t REGION_COUNT = 3;
	size_t service_count = 0;
	size_t payload_count = 0;
	create_tracker();

	for(size_t i=0; i < REGION_COUNT; ++i) {
		create_region();
	}

	for(size_t i=0; i < REGION_COUNT; ++i) {
		auto region = std::make_shared<RegionSession>();
		ASSERT_EQ(0,region->connect(getRegion(i).api_address));

		std::list<std::string> services;
		std::list<std::string> payloads;

		region_add_services(region,services,100);
		region_add_payloads(region,payloads,50000);
		service_count += 100;
		payload_count += 50000;
	}


	auto tracker = std::make_shared<TrackerSession>();
	ASSERT_EQ(0,tracker->connect(getTracker(0).api_address,500));


	sleep(6);
	check_tracker_statistics(tracker,REGION_COUNT,service_count,payload_count);
}

TEST_F(RISTest,RobinService) {
	static const size_t REGION_COUNT = 10;
	create_tracker();

	for(size_t i=0; i < REGION_COUNT; ++i) {
		create_region();
	}

	for(size_t i=0; i < REGION_COUNT; ++i) {
		auto region = std::make_shared<RegionSession>();
		ASSERT_EQ(0,region->connect(getRegion(i).api_address));

		std::list<std::string> services;
		std::list<std::string> payloads;

		region_add_payloads(region,payloads,500);

		std::stringstream ss;
		ss << "address-" << i;
		region->asyncNewService("test-service",ss.str());
	}


	auto tracker = std::make_shared<TrackerSession>();
	ASSERT_EQ(0,tracker->connect(getTracker(0).api_address,500));

	sleep(6);

	std::list<std::string> addrs;
	for(size_t i=0; i < REGION_COUNT; ++i) {
		RouteInfo ri;
		ASSERT_EQ(1,tracker->getServiceRouteInfo(&ri,"test-service"));
//		std::cout << ri.address << std::endl;
		ASSERT_EQ(addrs.end(),std::find(addrs.begin(),addrs.end(),ri.address));
		addrs.push_back(ri.address);
	}

}

TEST_F(RISTest,RegionOffline) {
	static const size_t REGION_COUNT = 2;
	static const size_t REGION_SERVICE_COUNT = 100;
	static const size_t REGION_PAYLOAD_COUNT = 500;

	size_t service_count = 0;
	size_t payload_count = 0;
	create_tracker();

	for(size_t i=0; i < REGION_COUNT; ++i) {
		create_region();
	}

	for(size_t i=0; i < REGION_COUNT; ++i) {
		auto region = std::make_shared<RegionSession>();
		ASSERT_EQ(0,region->connect(getRegion(i).api_address));

		std::list<std::string> services;
		std::list<std::string> payloads;

		region_add_services(region,services,REGION_SERVICE_COUNT);
		region_add_payloads(region,payloads,REGION_PAYLOAD_COUNT);
		service_count += REGION_SERVICE_COUNT;
		payload_count += REGION_PAYLOAD_COUNT;
	}


	auto tracker = std::make_shared<TrackerSession>();
	ASSERT_EQ(0,tracker->connect(getTracker(0).api_address,500));

	sleep(6);
	check_tracker_statistics(tracker,REGION_COUNT,service_count,payload_count);

	destroy_last_region();
	service_count -= REGION_SERVICE_COUNT;
	payload_count -= REGION_PAYLOAD_COUNT;
	sleep(1);

	check_tracker_statistics(tracker,REGION_COUNT - 1,service_count,payload_count);
}
