#pragma once

#include <string>
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

	inline const bus::BusHeader& header() const {
		return m_last_header;
	}

	std::shared_ptr<google::protobuf::Message> wait_pb();

	std::shared_ptr<google::protobuf::Message> reply_and_wait_pb(const google::protobuf::Message& reply);

	zmsg_t* reply_and_wait_z(zmsg_t** p_reply);

private:
	std::shared_ptr<google::protobuf::Message> do_wait_pb();
	zmsg_t* do_wait_z();
	int reconnect();
private:
	std::string				m_address;
	std::string				m_id;
	zsock_t*				m_sock;
	bus::BusHeader			m_last_header;
};


