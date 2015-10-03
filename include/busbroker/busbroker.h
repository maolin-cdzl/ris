#pragma once

#include <string>
#include <czmq.h>

#include "busbroker/busbrokerctx.h"

class BusBroker {
public:
	BusBroker(const std::shared_ptr<BusBrokerCtx>& ctx);
	~BusBroker();

	int run();

private:
	std::shared_ptr<BusBrokerCtx>			m_ctx;
};


