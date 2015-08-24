#pragma once

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif


int zmq_wait_readable(void* sock,long timeout);
int zmq_wait_writable(void* sock,long timeout);


#ifdef __cplusplus
}
#endif
