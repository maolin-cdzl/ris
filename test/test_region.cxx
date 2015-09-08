#include "mock_defines.h"

#include "region/regionapi.h"

extern zloop_t* g_loop;

class RegionTest : public testing::Test {
protected:
	static void SetUpTestCase() {
	}
	static void TearDownTestCase() {
	}


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

