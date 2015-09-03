#pragma once

#include <czmq.h>


class ReadableHelper {
public:
	ReadableHelper(zloop_t* loop);
	~ReadableHelper();

	inline zmsg_t* message() {
		return m_msg;
	}

	int register_read(zsock_t* sock);
	int register_read_int(zsock_t* sock);

	int unregister();
private:
	static int readAndInterrupt(zloop_t* loop,zsock_t* reader,void* arg);
	static int readAndGo(zloop_t* loop,zsock_t* reader,void* arg);

private:
	zloop_t*						m_loop;
	zsock_t*						m_sock;
	zmsg_t*							m_msg;
};

