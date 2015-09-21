#pragma once

#include <string>
#include <list>
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

	std::tuple<int,std::list<std::string>,std::shared_ptr<google::protobuf::Message>> wait_pb();

	std::tuple<int,std::list<std::string>,std::shared_ptr<google::protobuf::Message>> reply_and_wait_pb(const std::list<std::string>& targets,const std::list<std::string>& deny_targets,const google::protobuf::Message& reply);

	std::tuple<int,std::list<std::string>,zmsg_t*> reply_and_wait_z(const std::list<std::string>& targets,const std::list<std::string>& deny_targets,zmsg_t** p_reply);

private:
	std::tuple<int,std::list<std::string>,std::shared_ptr<google::protobuf::Message>> do_wait_pb();
	std::tuple<int,std::list<std::string>,zmsg_t*> do_wait_z();
	int reconnect();
private:
	std::string				m_address;
	std::string				m_id;
	zsock_t*				m_sock;
	uint64_t				m_last_msg_id;
};


