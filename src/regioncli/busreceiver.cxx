#include <glog/logging.h>
#include "regioncli/busreceiver.h"
#include "zmqx/zhelper.h"
#include "zmqx/zprotobuf++.h"

#define WORKER_READY		"\001"


// class BusReceiver
BusReceiver::BusReceiver() :
	m_id(new_short_identity()),
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

std::shared_ptr<google::protobuf::Message> BusReceiver::do_wait_pb() {
	do {
		if( -1 == zpb_recv(m_last_header,m_sock) ) {
			LOG(ERROR) << "Recv BusHeader failed";
			break;
		}

		if( ! zsock_rcvmore(m_sock) ) {
			LOG(ERROR) << "Bus message has no body";
			break;
		}

		auto msg = zpb_recv(m_sock);
		if( nullptr == msg ) {
			LOG(ERROR) << "Recv body failed";
			break;
		}

		return std::move(msg);
	} while(0);

	reconnect();
	return nullptr;
}

zmsg_t* BusReceiver::do_wait_z() {
	do {
		if( -1 == zpb_recv(m_last_header,m_sock) ) {
			LOG(ERROR) << "Recv BusHeader failed";
			break;
		}

		if( ! zsock_rcvmore(m_sock) ) {
			LOG(ERROR) << "Bus message has no body";
			break;
		}

		auto msg = zmsg_recv(m_sock);
		if( nullptr == msg ) {
			LOG(ERROR) << "Recv body failed";
			break;
		}

		return msg;
	} while(0);

	reconnect();
	return nullptr;
}

std::shared_ptr<google::protobuf::Message> BusReceiver::wait_pb() {
	CHECK_NOTNULL(m_sock);

	zframe_t* fr = zframe_new(WORKER_READY,1);
	if( -1 == zframe_send(&fr,m_sock,0) ) {
		if( fr ) {
			zframe_destroy(&fr);
		}
		reconnect();
		return nullptr;
	} else {
		return do_wait_pb();
	}
}

std::shared_ptr<google::protobuf::Message> BusReceiver::reply_and_wait_pb(const google::protobuf::Message& reply) {
	CHECK_NOTNULL(m_sock);
	CHECK( m_last_header.IsInitialized() );

	do {
		bus::BusRepHeader reph;
		reph.set_msg_id( m_last_header.msg_id() );
		std::copy(m_last_header.mutable_targets()->begin(),m_last_header.mutable_targets()->end(),google::protobuf::RepeatedFieldBackInserter(reph.mutable_targets()));
		m_last_header.Clear();

		if( -1 == zpb_sendm(m_sock,reph,false) ) {
			break;
		}
		if( -1 == zpb_send(m_sock,reply) ) {
			break;
		}
		return do_wait_pb();
	} while(0);

	reconnect();
	return nullptr;
}

zmsg_t* BusReceiver::reply_and_wait_z(zmsg_t** p_msg) {
	CHECK_NOTNULL(m_sock);

	do {
		bus::BusRepHeader reph;
		reph.set_msg_id( m_last_header.msg_id() );
		std::copy(m_last_header.mutable_targets()->begin(),m_last_header.mutable_targets()->end(),google::protobuf::RepeatedFieldBackInserter(reph.mutable_targets()));
		m_last_header.Clear();

		if( -1 == zpb_sendm(m_sock,reph,false) ) {
			break;
		}
		if( -1 == zmsg_send(p_msg,m_sock) ) {
			break;
		}
		return do_wait_z();
	} while(0);

	reconnect();
	return nullptr;
}

int BusReceiver::reconnect() {
	CHECK( !m_address.empty() );
	if( m_sock ) {
		zsock_destroy(&m_sock);
	}
	m_last_header.Clear();

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

