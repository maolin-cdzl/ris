#pragma once

#include <czmq.h>
#include "zmqx/zrunner.h"
#include "busbroker/busbrokerctx.h"

class BusBackend {
public:
	BusBackend(const std::shared_ptr<BusBrokerCtx>& ctx);
	~BusBackend();

	int start();
	void stop();

private:
	void run(zsock_t* pipe);
private:
	std::shared_ptr<BusBrokerCtx>			m_ctx;
	ZRunner									m_runner;
};

