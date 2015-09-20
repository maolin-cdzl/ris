#include <glog/logging.h>
#include "busreceiver.h"
#include "zmqx/zhelper.h"
#include "zmqx/zprotobuf++.h"

#define WORKER_READY		"\001"

// class BusMessage
BusMessage::BusMessage() :
	body(nullptr)
{
}

BusMessage::~BusMessage() {
	if( body ) {
		zmsg_destroy(&body);
	}
}


// class BusReceiver
BusReceiver::BusReceiver() :
	m_id(new_short_identitiy()),
	m_sock(nullptr)
{
}

BusReceiver::~BusReceiver() {
	disconnect();
}

int BusReceiver::connect(const std::string& address) {
	if( m_sock )
		return -1;

	m_address = address;
	return reconnect();
}

void BusReceiver::disconnect() {
	if( m_sock ) {
		zsock_destroy(&m_sock);
	}
	m_address.clear();
}

std::shared_ptr<BusMessage> BusReceiver::wait() {
	do {
		auto bmsg = std::make_shared<BusMessage>();

		if( -1 == zpb_recv(bmsg->header,m_sock) ) {
			LOG(ERROR) << "Recv BusHeader failed";
			break;
		}

		if( ! zsock_rcvmore(m_sock) ) {
			LOG(ERROR) << "Bus message has no body";
			break;
		}

		bmsg->body = zmsg_recv(m_sock);
		if( nullptr == bmsg->body ) {
			LOG(ERROR) << "Recv body failed";
		}

		return std::move(bmsg);
	} while(0);

	reconnect();
	return nullptr;
}

std::shared_ptr<BusMessage> BusReceiver::ready_and_wait() {
	CHECK_NOTNULL(m_sock);

	zframe_t* fr = zframe_new(WORKER_READY,1);
	if( -1 == zframe_send(&fr,m_sock,0) ) {
		if( fr ) {
			zframe_destroy(&fr);
		}
		reconnect();
		return nullptr;
	} else {
		return wait();
	}
}

std::shared_ptr<BusMessage> BusReceiver::reply_and_wait(const google::protobuf::Message& msg) {
	CHECK_NOTNULL(m_sock);
	if( -1 == zpb_send(m_sock,msg) ) {
		reconnect();
		return nullptr;
	} else {
		return wait();
	}
}

std::shared_ptr<BusMessage> BusReceiver::reply_and_wait(zmsg_t** p_msg) {
	CHECK_NOTNULL(m_sock);
	if( -1 == zmsg_send(p_msg,m_sock) ) {
		reconnect();
		return nullptr;
	} else {
		return wait();
	}
}

int BusReceiver::reconnect() {
	CHECK( !m_address.empty() );
	if( m_sock ) {
		zsock_destroy(&m_sock);
	}

	if( zsys_interrupted ) {
		return -1;
	}

	m_sock = zsock_new(ZMQ_REQ);
	CHECK_NOTNULL(m_sock);
	zsock_set_identity(m_sock,m_id.c_str());

	if( -1 == zsock_connect(m_sock,"%s",m_address.c_str()) ) {
		LOG(FATAL) << "Can not connect to: " << m_address;
	}
	return 0;
}

