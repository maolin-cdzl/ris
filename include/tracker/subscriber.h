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
	std::shared_ptr<sub_dispatcher_t> make_dispatcher();

	int defaultProcess(const std::shared_ptr<google::protobuf::Message>& msg,const std::string& envelope);
	int onRegion(const std::shared_ptr<google::protobuf::Message>& msg,const std::string& envelope);
	int onRmRegion(const std::shared_ptr<google::protobuf::Message>& msg,const std::string& envelope);
	int onService(const std::shared_ptr<google::protobuf::Message>& msg,const std::string& envelope);
	int onRmService(const std::shared_ptr<google::protobuf::Message>& msg,const std::string& envelope);
	int onPayload(const std::shared_ptr<google::protobuf::Message>& msg,const std::string& envelope);
	int onRmPayload(const std::shared_ptr<google::protobuf::Message>& msg,const std::string& envelope);
private:
	zloop_t*							m_loop;
	std::shared_ptr<IRIObserver>		m_observer;
	std::shared_ptr<ZLoopReader>		m_reader;
};

