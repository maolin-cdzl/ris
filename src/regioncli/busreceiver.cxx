#include <glog/logging.h>
#include "regioncli/busreceiver.h"
#include "zmqx/zhelper.h"
#include "zmqx/zprotobuf++.h"

#define WORKER_READY		"\001"


// class BusReceiver
BusReceiver::BusReceiver() :
	m_id(new_short_identity()),
	m_sock(nullptr),
	m_last_msg_id(0)
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
	m_last_msg_id = 0;
}

std::tuple<int,std::set<std::string>,std::shared_ptr<google::protobuf::Message>> BusReceiver::do_wait_pb() {
	std::tuple<int,std::set<std::string>,std::shared_ptr<google::protobuf::Message>> result;
	std::get<0>(result) = -1;
	do {
		bus::SendMessage header;
		if( -1 == zpb_recv(header,m_sock) ) {
			LOG(ERROR) << "Recv BusHeader failed";
			break;
		}

		if( ! zsock_rcvmore(m_sock) ) {
			LOG(ERROR) << "Bus message has no body";
			break;
		}

		std::get<2>(result) = zpb_recv(m_sock,true);
		if( nullptr == std::get<2>(result) ) {
			LOG(ERROR) << "Recv body failed";
			break;
		}

		std::get<0>(result) = 0;
		auto& p = std::get<1>(result);
		std::copy(header.payloads().begin(),header.payloads().end(),std::inserter(p,p.end()));
		return std::move(result);
	} while(0);

	reconnect();
	return std::move(result);
}

std::tuple<int,std::set<std::string>,zmsg_t*> BusReceiver::do_wait_z() {
	std::tuple<int,std::set<std::string>,zmsg_t*> result;
	std::get<0>(result) = -1;
	std::get<2>(result) = nullptr;

	do {
		bus::SendMessage header;
		if( -1 == zpb_recv(header,m_sock) ) {
			LOG(ERROR) << "Recv BusHeader failed";
			break;
		}

		if( ! zsock_rcvmore(m_sock) ) {
			LOG(ERROR) << "Bus message has no body";
			break;
		}

		std::get<2>(result) = zmsg_recv(m_sock);
		if( nullptr == std::get<2>(result) ) {
			LOG(ERROR) << "Recv body failed";
			break;
		}

		std::get<0>(result) = 0;
		auto& p = std::get<1>(result);
		std::copy(header.payloads().begin(),header.payloads().end(),std::inserter(p,p.end()));
		return std::move(result);
	} while(0);

	reconnect();
	return std::move(result);
}

std::tuple<int,std::set<std::string>,std::shared_ptr<google::protobuf::Message>> BusReceiver::wait_pb() {
	CHECK_NOTNULL(m_sock);

	std::tuple<int,std::set<std::string>,std::shared_ptr<google::protobuf::Message>> result;
	std::get<0>(result) = -1;

	zframe_t* fr = zframe_new(WORKER_READY,1);
	if( -1 == zframe_send(&fr,m_sock,0) ) {
		if( fr ) {
			zframe_destroy(&fr);
		}
		reconnect();
	} else {
		result = do_wait_pb();
	}
	return std::move(result);
}

std::tuple<int,std::set<std::string>,zmsg_t*> BusReceiver::wait_z() {
	CHECK_NOTNULL(m_sock);

	std::tuple<int,std::set<std::string>,zmsg_t*> result;
	std::get<0>(result) = -1;
	std::get<2>(result) = nullptr;

	zframe_t* fr = zframe_new(WORKER_READY,1);
	if( -1 == zframe_send(&fr,m_sock,0) ) {
		if( fr ) {
			zframe_destroy(&fr);
		}
		reconnect();
	} else {
		result = do_wait_z();
	}
	return std::move(result);
}

int BusReceiver::reply(const std::set<std::string>& payloads,const google::protobuf::Message& reply) {
	CHECK_NOTNULL(m_sock);

	if( m_last_msg_id == 0 )
		return -1;

	do {
		bus::ReplyMessage reph;
		reph.set_msg_id( m_last_msg_id );
		std::copy(payloads.begin(),payloads.end(),google::protobuf::RepeatedFieldBackInserter(reph.mutable_payloads()));

		if( -1 == zpb_sendm(m_sock,reph,false) ) {
			break;
		}
		if( -1 == zpb_send(m_sock,reply) ) {
			break;
		}
		return 0;
	} while(0);

	reconnect();
	return -1;
}

int BusReceiver::reply(const std::set<std::string>& payloads,zmsg_t** p_reply) {
	CHECK_NOTNULL(m_sock);

	if( m_last_msg_id == 0 )
		return -1;

	do {
		bus::ReplyMessage reph;
		reph.set_msg_id( m_last_msg_id );
		std::copy(payloads.begin(),payloads.end(),google::protobuf::RepeatedFieldBackInserter(reph.mutable_payloads()));

		if( -1 == zpb_sendm(m_sock,reph,false) ) {
			break;
		}
		if( -1 == zmsg_send(p_reply,m_sock) ) {
			break;
		}
		return 0;
	} while(0);

	reconnect();
	return -1;
}

int BusReceiver::reconnect() {
	CHECK( !m_address.empty() );
	if( m_sock ) {
		zsock_destroy(&m_sock);
	}
	m_last_msg_id = 0;

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

