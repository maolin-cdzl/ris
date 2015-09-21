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

uint64_t BusSender::send(const std::string& target,const google::protobuf::Message& msg,bool req_state) {
	CHECK_NOTNULL(m_sock);
	CHECK( ! target.empty() );
	CHECK( msg.IsInitialized() );

	do {
		bus::SendHeader header;
		header.set_msg_id(new_short_bin_identity());
		header.add_targets(target);
		header.set_req_state(req_state);

		if( -1 == zpb_sendm(m_sock,header,true) ) {
			break;
		}
		if( -1 == zpb_send(m_sock,msg,false) ) {
			break;
		}
		return header.msg_id();
	} while(0);

	return 0;
}

uint64_t BusSender::send(const std::list<std::string>& targets,const google::protobuf::Message& msg,bool req_state) {
	CHECK_NOTNULL(m_sock);
	CHECK( ! targets.empty() );
	CHECK( msg.IsInitialized() );

	do {
		bus::SendHeader header;
		header.set_msg_id(new_short_bin_identity());
		std::copy(targets.begin(),targets.end(),google::protobuf::RepeatedFieldBackInserter(header.mutable_targets()));
		header.set_req_state(req_state);

		if( -1 == zpb_sendm(m_sock,header,true) ) {
			break;
		}
		if( -1 == zpb_send(m_sock,msg,false) ) {
			break;
		}
		return header.msg_id();
	} while(0);

	return 0;
}

uint64_t BusSender::send(const std::string& target,zmsg_t** p_msg,bool req_state) {
	CHECK_NOTNULL(m_sock);
	CHECK( ! target.empty() );
	CHECK( p_msg );
	CHECK( *p_msg );

	do {
		bus::SendHeader header;
		header.set_msg_id(new_short_bin_identity());
		header.add_targets(target);
		header.set_req_state(req_state);

		if( -1 == zpb_sendm(m_sock,header,true) ) {
			break;
		}
		if( -1 == zmsg_send(p_msg,m_sock) ) {
			break;
		}
		return header.msg_id();
	} while(0);

	return 0;
}

uint64_t BusSender::send(const std::list<std::string>& targets,zmsg_t** p_msg,bool req_state) {
	CHECK_NOTNULL(m_sock);
	CHECK( ! targets.empty() );
	CHECK( p_msg );
	CHECK( *p_msg );

	do {
		bus::SendHeader header;
		header.set_msg_id(new_short_bin_identity());
		std::copy(targets.begin(),targets.end(),google::protobuf::RepeatedFieldBackInserter(header.mutable_targets()));
		header.set_req_state(req_state);

		if( -1 == zpb_sendm(m_sock,header,true) ) {
			break;
		}
		if( -1 == zmsg_send(p_msg,m_sock) ) {
			break;
		}
		return header.msg_id();
	} while(0);

	return 0;
}

std::tuple<int,std::list<std::string>,std::list<std::string>> BusSender::wait_send_state(uint64_t msg_id,uint64_t timeout) {
	uint64_t now = time_now();
	const uint64_t deadline = now + timeout;
	std::tuple<int,std::list<std::string>,std::list<std::string>> result;
	std::get<0>(result) = -1;

	while( now < deadline && zmq_wait_readable(m_sock,deadline - now) > 0 ) {
		bus::SendState state;
		if( 0 == zpb_recv(state,m_sock,true) ) {
			if( state.msg_id() == msg_id ) {
				std::get<0>(result) = 0;
				std::copy(state.targets().begin(),state.targets().end(),std::back_inserter( std::get<1>(result)) );
				std::copy(state.unreached_targets().begin(),state.unreached_targets().end(),std::back_inserter( std::get<2>(result) ));
				break;
			}
		}

		now = time_now();
	}

	return std::move(result);
}


std::tuple<int,std::list<std::string>,std::list<std::string>,std::shared_ptr<google::protobuf::Message>> BusSender::wait_pb_reply(uint64_t msg_id,uint64_t timeout) {
	CHECK_NOTNULL(m_sock);
	CHECK_GE(timeout,1);

	uint64_t now = time_now();
	const uint64_t deadline = now + timeout;
	std::tuple<int,std::list<std::string>,std::list<std::string>,std::shared_ptr<google::protobuf::Message>> result;
	std::get<0>(result) = -1;

	while( now < deadline && zmq_wait_readable(m_sock,deadline - now) > 0 ) {
		bus::ReplyHeader header;
		if( 0 == zpb_recv(header,m_sock) ) {
			if( header.msg_id() == msg_id ) {
				if( zsock_rcvmore(m_sock) ) {
					auto reply = zpb_recv(m_sock,true);
					if( reply ) {
						std::get<0>(result) = 0;
						std::copy(header.targets().begin(),header.targets().end(),std::back_inserter(std::get<1>(result)));
						std::copy(header.deny_targets().begin(),header.deny_targets().end(),std::back_inserter(std::get<2>(result)));
						std::get<3>(result) = reply;
					}
				}
				break;
			}
		}

		now = time_now();
	}

	return std::move(result);
}

std::tuple<int,std::list<std::string>,std::list<std::string>,zmsg_t*> BusSender::wait_zmq_reply(uint64_t msg_id,uint64_t timeout) {
	CHECK_NOTNULL(m_sock);
	CHECK_GE(timeout,1);

	uint64_t now = time_now();
	const uint64_t deadline = now + timeout;
	std::tuple<int,std::list<std::string>,std::list<std::string>,zmsg_t*> result;
	std::get<0>(result) = -1;

	while( now < deadline && zmq_wait_readable(m_sock,deadline - now) > 0 ) {
		bus::ReplyHeader header;
		if( 0 == zpb_recv(header,m_sock) ) {
			if( header.msg_id() == msg_id ) {
				if( zsock_rcvmore(m_sock) ) {
					auto reply = zmsg_recv(m_sock);
					if( reply ) {
						std::get<0>(result) = 0;
						std::copy(header.targets().begin(),header.targets().end(),std::back_inserter(std::get<1>(result)));
						std::copy(header.deny_targets().begin(),header.deny_targets().end(),std::back_inserter(std::get<2>(result)));
						std::get<3>(result) = reply;
					}
				}
				break;
			} else {
				zsock_flush(m_sock);
			}
		}

		now = time_now();
	}

	return std::move(result);
}


bool BusSender::checked_send(const std::string& target,const google::protobuf::Message& msg,uint64_t timeout) {
	do {
		const uint64_t msg_id = send(target,msg,true);
		if( 0 == msg_id ) {
			break;
		}

		auto state = wait_send_state(msg_id,timeout);
		if( std::get<0>(state) == 0 ) {
			return !std::get<1>(state).empty();
		}
	} while(0);

	return false;
}


bool BusSender::checked_send(const std::string& target,zmsg_t** p_msg,uint64_t timeout) {
	do {
		const uint64_t msg_id = send(target,p_msg,true);
		if( 0 == msg_id ) {
			break;
		}

		auto state = wait_send_state(msg_id,timeout);
		if( std::get<0>(state) == 0 ) {
			return ! std::get<1>(state).empty();
		}
	} while(0);

	return false;
}

std::list<std::string> BusSender::checked_send(const std::list<std::string>& targets,const google::protobuf::Message& msg,uint64_t timeout) {
	do {
		const uint64_t msg_id = send(targets,msg,true);
		if( 0 == msg_id ) {
			break;
		}

		auto state = wait_send_state(msg_id,timeout);
		if( std::get<0>(state) == 0 ) {
			return std::get<1>(state);
		}
	} while(0);

	return std::list<std::string>();
}

std::list<std::string> BusSender::checked_send(const std::list<std::string>& targets,zmsg_t** p_msg,uint64_t timeout) {
	do {
		const uint64_t msg_id = send(targets,p_msg,true);
		if( 0 == msg_id ) {
			break;
		}

		auto state = wait_send_state(msg_id,timeout);
		if( std::get<0>(state) == 0 ) {
			return std::get<1>(state);
		}
	} while(0);

	return std::list<std::string>();
}



