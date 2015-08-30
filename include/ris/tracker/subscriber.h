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

	int setObserver(const std::shared_ptr<IRIObserver>& ob);
private:
	int startLoop();
	void stopLoop();

	void defaultProcess(const std::shared_ptr<google::protobuf::Message>& msg,int err);
	void onRegion(const std::shared_ptr<google::protobuf::Message>& msg);
	void onRmRegion(const std::shared_ptr<google::protobuf::Message>& msg);
	void onService(const std::shared_ptr<google::protobuf::Message>& msg);
	void onRmService(const std::shared_ptr<google::protobuf::Message>& msg);
	void onPayload(const std::shared_ptr<google::protobuf::Message>& msg);
	void onRmPayload(const std::shared_ptr<google::protobuf::Message>& msg);
private:
	zloop_t*							m_loop;
	zsock_t*							m_sub;
	std::shared_ptr<IRIObserver>		m_observer;
	std::shared_ptr<ZDispatcher>		m_disp;
};

