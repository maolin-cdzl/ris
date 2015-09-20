#pragma once

#include <string>
#include <memory>
#include <czmq.h>

#include "ris/bus.pb.h"

class BusMessage {
public:
	bus::BusHeader		header;
	zmsg_t*				body;

	BusMessage();
	~BusMessage();
private:
	BusMessage(const BusMessage&) = delete;
	BusMessage& operator = (const BusMessage&) = delete;
};

class BusReceiver {
public:
	BusReceiver();
	~BusReceiver();

	int connect(const std::string& address);
	void disconnect();

	inline bool good() const {
		return m_sock != nullptr;
	}

	std::shared_ptr<BusMessage> ready_and_wait();

	std::shared_ptr<BusMessage> reply_and_wait(const google::protobuf::Message& msg);

	std::shared_ptr<BusMessage> reply_and_wait(zmsg_t** p_msg);

private:
	std::shared_ptr<BusMessage> wait();
	int reconnect();
private:
	std::string				m_address;
	std::string				m_id;
	zsock_t*				m_sock;
};


