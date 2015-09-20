#pragma once

#include <string>
#include <list>
#include <memory>
#include <czmq.h>
#include <google/protobuf/message.h>

class BusSender {
public:
	BusSender();
	~BusSender();

	int connect(const std::list<std::string>& brokers);
	void disconnect();

	inline bool good() const {
		return m_sock != nullptr;
	}

	int send(const std::string& target,const google::protobuf::Message& msg);
	int send(const std::list<std::string>& targets,const google::protobuf::Message& msg);
	int send(const std::string& target,zmsg_t** p_msg);
	int send(const std::list<std::string>& targets,zmsg_t** p_msg);

	
	std::shared_ptr<google::protobuf::Message> wait_pb_reply(uint64_t timeout=1000);

	int wait_pb_reply(google::protobuf::Message& msg,uint64_t timeout=1000);

	zmsg_t* wait_zmq_reply(uint64_t timeout=1000);

private:
	std::string				m_id;
	zsock_t*				m_sock;
	std::list<std::string>	m_brokers;
};

