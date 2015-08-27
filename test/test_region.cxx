#include <unistd.h>
#include "../regionapi/regionapi.h"

int main(int argc,char* argv[]) {
	int result = region_start(CONFI_FILE,1);
	if( -1 == result )
		return -1;

	void* s = region_open();

	char id[32];
	for(size_t i=1; i <= 50000; ++i) {
		sprintf(id,"%lu",i);
		region_new_payload(s,id);
	}
	region_close(s);
	sleep(300);
	region_stop();
	return 0;
}

