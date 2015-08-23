#pragma once

#include <czmq.h>


class ILoopable {
public:
	virtual int startLoop(zloop_t* loop) = 0;
	virtual void stopLoop(zloop_t* loop) = 0;
};
