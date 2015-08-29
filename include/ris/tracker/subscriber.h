#pragma once

#include <string>
#include "ris/riobserver.h"
#include "zmqx/zdispatcher.h"

class RISubscriber {
public:
	RISubscriber(zloop_t* loop);
	~RISubscriber();

	int start(const std::string& address,const std::shared_ptr<IRIObserver>& ob);
	int stop();
private:
	int onSubReadable();
	static int subReadableAdapter(zloop_t* loop,zsock_t* reader,void* arg);

	static void defaultAdapter(const std::shared_ptr<google::protobuf::Message>& msg,int err,void* args);
	void (*pb_msg_process_fn)(const std::shared_ptr<google::protobuf::Message>& msg,void* args);
private:
	zloop_t*							m_loop;
	zsock_t*							m_sub;
	std::shared_ptr<IRIObserver>		m_observer;
	std::shared_ptr<ZDispatcher>		m_disp;
};

