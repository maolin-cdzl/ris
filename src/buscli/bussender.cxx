#include <glog/logging.h>
#include "buscli/bussender.h"
#include "zmqx/zhelper.h"
#include "zmqx/zprotobuf++.h"
#include "ris/bus.pb.h"

BusSender::BusSender() :
	m_id(new_short_identity()),
	m_sock(nullptr)
{
}

BusSender::~BusSender() {
	disconnect();
}

int BusSender::connect(const std::list<std::string>& brokers) {
	CHECK( ! brokers.empty() );
	if( m_sock ) {
		return -1;
	}

	m_sock = zsock_new(ZMQ_DEALER);
	CHECK_NOTNULL(m_sock);
	zsock_set_identity(m_sock,m_id.c_str());

	for(auto it=brokers.begin(); it != brokers.end(); ++it) {
		if( -1 == zsock_connect(m_sock,"%s",it->c_str()) ) {
			LOG(FATAL) << "Can not connect to: " << *it;
			break;
		}
	}

	m_brokers = brokers;
	return 0;
}

void BusSender::disconnect() {
	if( m_sock ) {
		zsock_destroy(&m_sock);
	}
	m_brokers.clear();
}

int BusSender::send(const std::string& target,const google::protobuf::Message& msg) {
	CHECK_NOTNULL(m_sock);
	CHECK( ! target.empty() );
	CHECK( msg.IsInitialized() );

	bus::BusHeader header;
	header.set_msg_id(new_short_bin_identity());
	header.add_targets(target);

	do {
		if( -1 == zpb_sendm(m_sock,header,true) ) {
			break;
		}
		if( -1 == zpb_send(m_sock,msg,false) ) {
			break;
		}
		return 0;
	} while(0);

	return -1;
}

int BusSender::send(const std::list<std::string>& targets,const google::protobuf::Message& msg) {
	CHECK_NOTNULL(m_sock);
	CHECK( ! targets.empty() );
	CHECK( msg.IsInitialized() );

	bus::BusHeader header;
	header.set_msg_id(new_short_bin_identity());
	std::copy(targets.begin(),targets.end(),google::protobuf::RepeatedFieldBackInserter(header.mutable_targets()));

	do {
		if( -1 == zpb_sendm(m_sock,header,true) ) {
			break;
		}
		if( -1 == zpb_send(m_sock,msg,false) ) {
			break;
		}
		return 0;
	} while(0);

	return -1;
}

int BusSender::send(const std::string& target,zmsg_t** p_msg) {
	CHECK_NOTNULL(m_sock);
	CHECK( ! target.empty() );
	CHECK( p_msg );
	CHECK( *p_msg );

	bus::BusHeader header;
	header.set_msg_id(new_short_bin_identity());
	header.add_targets(target);

	do {
		if( -1 == zpb_sendm(m_sock,header,true) ) {
			break;
		}
		if( -1 == zmsg_send(p_msg,m_sock) ) {
			break;
		}
		return 0;
	} while(0);

	return -1;
}

int BusSender::send(const std::list<std::string>& targets,zmsg_t** p_msg) {
	CHECK_NOTNULL(m_sock);
	CHECK( ! targets.empty() );
	CHECK( p_msg );
	CHECK( *p_msg );

	bus::BusHeader header;
	header.set_msg_id(new_short_bin_identity());
	std::copy(targets.begin(),targets.end(),google::protobuf::RepeatedFieldBackInserter(header.mutable_targets()));

	do {
		if( -1 == zpb_sendm(m_sock,header,true) ) {
			break;
		}
		if( -1 == zmsg_send(p_msg,m_sock) ) {
			break;
		}
		return 0;
	} while(0);

	return -1;
}


std::shared_ptr<google::protobuf::Message> BusSender::wait_pb_reply(uint64_t timeout) {
	CHECK_NOTNULL(m_sock);
	CHECK_GE(timeout,1);

	do {
		if( zmq_wait_readable(m_sock,timeout) <= 0 ) {
			break;
		}
		return zpb_recv(m_sock);
	} while(0);

	return nullptr;
}

int BusSender::wait_pb_reply(google::protobuf::Message& msg,uint64_t timeout) {
	CHECK_NOTNULL(m_sock);
	CHECK_GE(timeout,1);

	do {
		if( zmq_wait_readable(m_sock,timeout) <= 0 ) {
			break;
		}
		return zpb_recv(msg,m_sock);
	} while(0);

	return -1;
}


zmsg_t* BusSender::wait_zmq_reply(uint64_t timeout) {
	CHECK_NOTNULL(m_sock);
	CHECK_GE(timeout,1);

	do {
		if( zmq_wait_readable(m_sock,timeout) <= 0 ) {
			break;
		}
		return zmsg_recv(m_sock);
	} while(0);

	return nullptr;
}

