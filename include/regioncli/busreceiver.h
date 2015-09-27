#pragma once

#include <string>
#include <set>
#include <memory>
#include <czmq.h>

#include <google/protobuf/message.h>
#include "ris/bus.pb.h"

class BusReceiver {
public:
	BusReceiver();
	~BusReceiver();

	int connect(const std::string& address);
	void disconnect();

	inline bool good() const {
		return m_sock != nullptr;
	}

	std::tuple<int,std::set<std::string>,std::shared_ptr<google::protobuf::Message>> wait_pb();
	std::tuple<int,std::set<std::string>,zmsg_t*> wait_z();

	int reply(const std::set<std::string>& payloads,const google::protobuf::Message& reply);

	int reply(const std::set<std::string>& payloads,zmsg_t** p_reply);

private:
	std::tuple<int,std::set<std::string>,std::shared_ptr<google::protobuf::Message>> do_wait_pb();
	std::tuple<int,std::set<std::string>,zmsg_t*> do_wait_z();
	int reconnect();
private:
	std::string				m_address;
	std::string				m_id;
	zsock_t*				m_sock;
	uint64_t				m_last_msg_id;
};


