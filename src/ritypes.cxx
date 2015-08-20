#include <time.h>
#include "ris/ritypes.h"


ri_time_t ri_time_now() {
	ri_time_t now;
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC,&ts);

	now = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
	return now;
}

