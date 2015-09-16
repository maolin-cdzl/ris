#pragma once

#include <string>
#include "ris/riobserver.h"
#include "zmqx/zpbreader.h"

class RISubscriber {
public:
	RISubscriber(zloop_t* loop);
	~RISubscriber();

	int start(const std::string& address,const std::shared_ptr<IRIObserver>& ob);
	void stop();

	int setObserver(const std::shared_ptr<IRIObserver>& ob);
private:
	std::shared_ptr<sock_dispatcher_t> make_dispatcher();

	int defaultProcess(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock);
	int onRegion(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock);
	int onRmRegion(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock);
	int onService(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock);
	int onRmService(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock);
	int onPayload(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock);
	int onRmPayload(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock);
private:
	zloop_t*							m_loop;
	std::shared_ptr<IRIObserver>		m_observer;
	std::shared_ptr<ZLoopReader>		m_reader;
};

