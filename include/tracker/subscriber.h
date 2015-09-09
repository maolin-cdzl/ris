#pragma once

#include <string>
#include "ris/riobserver.h"
#include "zmqx/zdispatcher.h"

class RISubscriber {
public:
	RISubscriber(zloop_t* loop);
	~RISubscriber();

	int start(const std::string& address,const std::shared_ptr<IRIObserver>& ob);
	void stop();

	int setObserver(const std::shared_ptr<IRIObserver>& ob);
private:
	std::shared_ptr<Dispatcher> make_dispatcher();

	int defaultProcess(const std::shared_ptr<google::protobuf::Message>& msg,int err);
	int onRegion(const std::shared_ptr<google::protobuf::Message>& msg);
	int onRmRegion(const std::shared_ptr<google::protobuf::Message>& msg);
	int onService(const std::shared_ptr<google::protobuf::Message>& msg);
	int onRmService(const std::shared_ptr<google::protobuf::Message>& msg);
	int onPayload(const std::shared_ptr<google::protobuf::Message>& msg);
	int onRmPayload(const std::shared_ptr<google::protobuf::Message>& msg);
private:
	zloop_t*							m_loop;
	std::shared_ptr<IRIObserver>		m_observer;
	std::shared_ptr<ZDispatcher>		m_disp;
};

