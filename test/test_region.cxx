#include <unistd.h>
#include <time.h>
#include <iostream>
#include "../regionapi/regionapi.h"

static uint64_t t_time_now();

int main(int argc,char* argv[]) {
	int result = region_start(CONFI_FILE,1);
	if( -1 == result )
		return -1;

	void* s = region_open();

	char id[32];
	const uint64_t tv_start = t_time_now();
	for(size_t i=1; i <= 50000; ++i) {
		sprintf(id,"%lu",i);
		region_new_payload(s,id);
	}
	const uint64_t tv_end = t_time_now();
	std::cout << "Cost " << tv_end - tv_start << " msec" << std::endl;
	region_close(s);
	sleep(30);
	region_stop();
	return 0;
}

static uint64_t t_time_now() {
	uint64_t now;
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC,&ts);

	now = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
	return now;
}

