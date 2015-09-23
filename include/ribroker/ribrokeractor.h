#pragma once

#include <pthread.h>
#include <zmq.h>
#include <string>

class RIBrokerActor {
public:
	RIBrokerActor();
	~RIBrokerActor();

	int start(const std::string& frontend,const std::string& backend);
	void stop();
private:
	static void* broker_routine(void* arg);
private:
	pthread_t				m_thread;
	void*					m_ctx;
	std::string				m_front_addr;
	std::string				m_back_addr;
};

