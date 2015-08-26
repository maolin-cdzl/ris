#include <unistd.h>
#include "ris/region/regiondll.h"

int main(int argc,char* argv[]) {
	int result = region_start(CONFI_FILE,1);
	if( -1 == result )
		return -1;

	sleep(5);

	region_stop();
	return 0;
}

