#pragma once

#include <string>
#include <list>
#include <memory>
#include <czmq.h>
#include <google/protobuf/message.h>

struct SendState {
	std::list<std::string>			targets;
	std::list<std::string>			unreached_targets;
};

class BusSender {
public:
	BusSender();
	~BusSender();

	int connect(const std::list<std::string>& brokers);
	void disconnect();

	inline bool good() const {
		return m_sock != nullptr;
	}

	// low level interface
	// return message id, or 0 if failed.
	uint64_t send(const std::string& target,const google::protobuf::Message& msg,bool req_state=false);
	uint64_t send(const std::list<std::string>& targets,const google::protobuf::Message& msg,bool req_state=false);
	uint64_t send(const std::string& target,zmsg_t** p_msg,bool req_state=false);
	uint64_t send(const std::list<std::string>& targets,zmsg_t** p_msg,bool req_state=false);

	std::tuple<int,std::list<std::string>,std::list<std::string>> wait_send_state(uint64_t msg_id,uint64_t timeout);

	std::tuple<int,std::list<std::string>,std::list<std::string>,std::shared_ptr<google::protobuf::Message>> wait_pb_reply(uint64_t msg_id,uint64_t timeout=1000);

	std::tuple<int,std::list<std::string>,std::list<std::string>,zmsg_t*> wait_zmq_reply(uint64_t msg_id,uint64_t timeout=1000);

	// combine interface
	bool checked_send(const std::string& target,const google::protobuf::Message& msg,uint64_t timeout=1000);
	bool checked_send(const std::string& target,zmsg_t** p_msg,uint64_t timeout=1000);

	std::list<std::string> checked_send(const std::list<std::string>& targets,const google::protobuf::Message& msg,uint64_t timeout=1000);
	std::list<std::string> checked_send(const std::list<std::string>& targets,zmsg_t** p_msg,uint64_t timeout=1000);
	

	// send and wait reply from targets

private:
private:
	std::string				m_id;
	zsock_t*				m_sock;
	std::list<std::string>	m_brokers;
};

