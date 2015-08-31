#include <unistd.h>
#include <iostream>
#include "../trackerapi/trackerapi.h"


int main(int argc,char* argv[]) {
	int result = tracker_start(CONFI_FILE,1);
	if( -1 == result )
		return -1;

	tracker_wait();
	return 0;
}

