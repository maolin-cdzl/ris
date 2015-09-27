#pragma once

#include <string>
#include <list>
#include <set>
#include <memory>
#include <czmq.h>
#include <google/protobuf/message.h>

#include "buscli/busmsgreply.h"

class BusSender {
public:
	BusSender();
	~BusSender();

	int connect(const std::list<std::string>& brokers);
	void disconnect();

	inline bool good() const {
		return m_sock != nullptr;
	}

	bool send(const std::string& payload,const google::protobuf::Message& msg);
	bool send(const std::set<std::string>& payloads,const google::protobuf::Message& msg);
	bool send(const std::string& payload,zmsg_t** p_msg);
	bool send(const std::set<std::string>& payloads,zmsg_t** p_msg);

	std::shared_ptr<BusMsgReply> sendWaitReply(const std::string& payload,const google::protobuf::Message& msg,uint64_t timeout=1000);
	std::shared_ptr<BusMsgReply> sendWaitReply(const std::set<std::string>& payloads,const google::protobuf::Message& msg,uint64_t timeout=1000);
	std::shared_ptr<BusMsgReply> sendWaitReply(const std::string& payload,zmsg_t** p_msg,uint64_t timeout=1000);
	std::shared_ptr<BusMsgReply> sendWaitReply(const std::set<std::string>& payloads,zmsg_t** p_msg,uint64_t timeout=1000);
private:
	// low level interface
	// return message id, or 0 if failed.
	uint64_t do_send(const std::string& payload,const google::protobuf::Message& msg,bool failure=false);
	uint64_t do_send(const std::set<std::string>& payloads,const google::protobuf::Message& msg,bool failure=false);
	uint64_t do_send(const std::string& payload,zmsg_t** p_msg,bool failure=false);
	uint64_t do_send(const std::set<std::string>& payloads,zmsg_t** p_msg,bool failure=false);

	std::shared_ptr<BusMsgReply> wait_reply(uint64_t msg_id,const std::set<std::string>& payloads,uint64_t timeout=1000);
private:
	std::string				m_id;
	zsock_t*				m_sock;
	std::list<std::string>	m_brokers;
};

