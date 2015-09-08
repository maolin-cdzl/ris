#include "mock_defines.h"

#include "region/regionapi.h"
#include "tracker/trackerapi.h"
#include "trackersession.h"

extern zloop_t* g_loop;

static const char* REGION_CONFIG =
"region:\n"
"{\n"
"	id = \"test-001\";\n"
"	idc = \"test-idc\";\n"
"	api_address = \"inproc://region-test-001\";\n"
"	bus_address = \"tcp://127.0.0.1:6600\";\n"
"\n"
"	#note,epgm protocol did NOT support publisher and subscriber on single host!\n"
"	#pub_address = \"epgm://eth0;239.192.1.1:5000\";\n"
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
"#note,epgm protocol did NOT support publisher and subscriber on single host!\n"
"#pub_address = \"epgm://eth0;239.192.1.1:5000\";\n"
"	pub_address = \"tcp://127.0.0.1:2016\";\n"
"\n"
"	snapshot:\n"
"	{\n"
"		address = \"tcp://127.0.0.1:7500\";\n"
"		worker_address = \"tcp://127.0.0.1:![7501-7599]\";\n"
"	};\n"
"\n"
"};\n";



class RegionTest : public testing::Test {
protected:
	static void SetUpTestCase() {
		int err = -1;
		err = tracker_start_str(TRACKER_CONFIG);
		assert(err == 0);

		err = region_start_str(REGION_CONFIG);
		assert(err == 0);

		trk_session = new TrackerSession();
	}
	static void TearDownTestCase() {
	}


	static void*					reg_session;
	static TrackerSession*			trk_session;
};

/*
int main(int argc,char* argv[]) {
	int result = region_start(CONFI_FILE,1);
	if( -1 == result )
		return -1;

	void* s = region_open();

	char id[32];
	const uint64_t tv_start = t_time_now();
	for(size_t i=1; i <= PAYLOAD_SIZE; ++i) {
		sprintf(id,"%lu",i);
		region_new_payload(s,id);
	}
	const uint64_t tv_end = t_time_now();
	std::cout << "Cost " << tv_end - tv_start << " msec" << std::endl;
	region_close(s);
	region_wait();
	return 0;
}
*/

