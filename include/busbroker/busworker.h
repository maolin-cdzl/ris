#pragma once

#include <string>
#include <czmq.h>
#include "zmqx/zrunner.h"

class BusWorker {
public:
	BusWorker();
	~BusWorker();

	int start(const std::string& tracker,const std::string& frontend,const std::string& backend);
	void stop();

private:
	void run(const std::string& tk_addr,const std::string& ft_addr,const std::string& be_addr,zsock_t* pipe);
private:
	ZRunner				m_runner;
};


