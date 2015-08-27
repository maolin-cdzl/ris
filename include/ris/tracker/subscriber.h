#pragma once

#include <string>
#include <list>
#include "ris/loopable.h"
#include "ris/riobserver.h"

class RISubscriber {
public:
	RISubscriber(zloop_t* loop);
	~RISubscriber();

	int start(const std::list<std::string>& brokers,const std::shared_ptr<IRIObserver>& ob);
	int stop();
private:
	int onSubReadable();
	static int subReadableAdapter(zloop_t* loop,zsock_t* reader,void* arg);
private:
	zloop_t*							m_loop;
	zsock_t*							m_sub;
	std::list<std::string>				m_brokers;
	std::shared_ptr<IRIObserver>		m_observer;
};

