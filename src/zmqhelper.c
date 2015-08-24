#include "ris/zmqhelper.h"
#include <assert.h>

int zmq_wait_writable(void* sock,long timeout) {
	zmq_pollitem_t pi[1];

	assert(sock);

	pi[0].socket = zsock_resolve(sock);
	pi[0].fd = 0;
	pi[0].events = ZMQ_POLLOUT;

	return zmq_poll(pi,1,timeout);
}

int zmq_wait_readable(void* sock,long timeout) {
	zmq_pollitem_t pi[1];

	assert(sock);

	pi[0].socket = zsock_resolve(sock);
	pi[0].fd = 0;
	pi[0].events = ZMQ_POLLIN;

	return zmq_poll(pi,1,timeout);
}

