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

bool BusSender::send(const std::string& payload,const google::protobuf::Message& msg) {
	return 0 != do_send(payload,msg,false);
}

bool BusSender::send(const std::set<std::string>& payloads,const google::protobuf::Message& msg) {
	return 0 != do_send(payloads,msg);
}

bool BusSender::send(const std::string& payload,zmsg_t** p_msg) {
	return 0 != do_send(payload,p_msg);
}

bool BusSender::send(const std::set<std::string>& payloads,zmsg_t** p_msg) {
	return 0 != do_send(payloads,p_msg);
}

uint64_t BusSender::do_send(const std::string& payload,const google::protobuf::Message& msg,bool failure) {
	CHECK_NOTNULL(m_sock);
	CHECK( ! payload.empty() );
	CHECK( msg.IsInitialized() );

	do {
		bus::SendMessage header;
		header.set_msg_id(new_short_bin_identity());
		header.add_payloads(payload);
		header.set_failure(failure);

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

uint64_t BusSender::do_send(const std::set<std::string>& payloads,const google::protobuf::Message& msg,bool failure) {
	CHECK_NOTNULL(m_sock);
	CHECK( ! payloads.empty() );
	CHECK( msg.IsInitialized() );

	do {
		bus::SendMessage header;
		header.set_msg_id(new_short_bin_identity());
		std::copy(payloads.begin(),payloads.end(),google::protobuf::RepeatedFieldBackInserter(header.mutable_payloads()));
		header.set_failure(failure);

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

uint64_t BusSender::do_send(const std::string& payload,zmsg_t** p_msg,bool failure) {
	CHECK_NOTNULL(m_sock);
	CHECK( ! payload.empty() );
	CHECK( p_msg );
	CHECK( *p_msg );

	do {
		bus::SendMessage header;
		header.set_msg_id(new_short_bin_identity());
		header.add_payloads(payload);
		header.set_failure(failure);

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

uint64_t BusSender::do_send(const std::set<std::string>& payloads,zmsg_t** p_msg,bool failure) {
	CHECK_NOTNULL(m_sock);
	CHECK( ! payloads.empty() );
	CHECK( p_msg );
	CHECK( *p_msg );

	do {
		bus::SendMessage header;
		header.set_msg_id(new_short_bin_identity());
		std::copy(payloads.begin(),payloads.end(),google::protobuf::RepeatedFieldBackInserter(header.mutable_payloads()));
		header.set_failure(failure);

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

std::shared_ptr<BusMsgReply> BusSender::sendWaitReply(const std::string& payload,const google::protobuf::Message& msg,uint64_t timeout) {
	const uint64_t msg_id = do_send(payload,msg,true);
	if( 0 != msg_id ) {
		std::set<std::string> payloads;
		payloads.insert(payload);
		return wait_reply(msg_id,payloads,timeout);
	}

	return nullptr;
}

std::shared_ptr<BusMsgReply> BusSender::sendWaitReply(const std::set<std::string>& payloads,const google::protobuf::Message& msg,uint64_t timeout) {
	const uint64_t msg_id = do_send(payloads,msg,true);
	if( 0 != msg_id ) {
		return wait_reply(msg_id,payloads,timeout);
	}

	return nullptr;
}

std::shared_ptr<BusMsgReply> BusSender::sendWaitReply(const std::string& payload,zmsg_t** p_msg,uint64_t timeout) {
	const uint64_t msg_id = do_send(payload,p_msg,true);
	if( 0 != msg_id ) {
		std::set<std::string> payloads;
		payloads.insert(payload);
		return wait_reply(msg_id,payloads,timeout);
	}

	return nullptr;
}

std::shared_ptr<BusMsgReply> BusSender::sendWaitReply(const std::set<std::string>& payloads,zmsg_t** p_msg,uint64_t timeout) {
	const uint64_t msg_id = do_send(payloads,p_msg,true);
	if( 0 != msg_id ) {
		return wait_reply(msg_id,payloads,timeout);
	}

	return nullptr;
}




std::shared_ptr<BusMsgReply> BusSender::wait_reply(uint64_t msg_id,const std::set<std::string>& payloads,uint64_t timeout) {
	auto reply = std::make_shared<BusMsgReply>(msg_id,payloads);
	uint64_t now = time_now();
	const uint64_t deadline = now + timeout;

	while( !reply->isCompleted() && now < deadline && zmq_wait_readable(m_sock,deadline - now) > 0 ) {
		do {
			auto msg = zpb_recv(m_sock,false);
			if( msg ) {
				if( msg->GetDescriptor() == bus::Failure::descriptor() ) {
					reply->addFailure(*std::dynamic_pointer_cast<bus::Failure>(msg));
				} else if( msg->GetDescriptor() == bus::ReplyMessage::descriptor() ) {
					if( zsock_rcvmore(m_sock) ) {
						zmsg_t* data = zmsg_recv(m_sock);
						if( data ) {
							reply->addReply(*std::dynamic_pointer_cast<bus::ReplyMessage>(msg),&data);
							if( data ) {
								zmsg_destroy(&data);
							}
							break;
						}
					}
				}
			}
			
			zsock_flush(m_sock);
		} while(0);

		now = time_now();
	}

	return std::move(reply);
}


