#pragma once

#include <string>
#include <czmq.h>
#include "zmqx/zrunner.h"

#include "busbroker/busbrokerctx.h"

class BusWorker {
public:
	BusWorker(const std::shared_ptr<BusBrokerCtx>& ctx);
	~BusWorker();

	int start();
	void stop();

private:
	void run(zsock_t* pipe);
private:
	std::shared_ptr<BusBrokerCtx>	m_ctx;
	ZRunner				m_runner;
};


