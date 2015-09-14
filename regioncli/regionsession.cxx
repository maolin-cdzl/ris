#include <glog/logging.h>
#include "regionsession.h"
#include "ris/regionapi.pb.h"
#include "zmqx/zprotobuf++.h"
#include "zmqx/zhelper.h"
#include "zmqx/zprepend.h"

RegionSession::RegionSession() :
	m_req(nullptr)
{
}

RegionSession::~RegionSession() {
	disconnect();
}

int RegionSession::connect(const std::string& api_address,uint64_t timeout) {
	if( m_req )
		return -1;

	do {
		m_req = zsock_new(ZMQ_DEALER);
		CHECK_NOTNULL(m_req);
		if( -1 == zsock_connect(m_req,"%s",api_address.c_str()) ) {
			LOG(ERROR) << "Error when connect to: " << api_address;
			break;
		}

		region::api::HandShake hs;
		if( -1 == zpb_send(m_req,hs,true) ) {
			LOG(ERROR) << "Error when sending HandShake to region";
			break;
		}
		
		if( zmq_wait_readable(m_req,timeout) <= 0 ) {
			LOG(ERROR) << "Error when waiting req readable";
			break;
		}
		
		if( -1 == zpb_recv(hs,m_req) ) {
			LOG(ERROR) << "Error when recv HandShake from req";
			break;
		}
		return 0;
	} while(0);

	if( m_req ) {
		zsock_destroy(&m_req);
	}
	return -1;
}

void RegionSession::disconnect() {
	if( m_req ) {
		zsock_destroy(&m_req);
	}
}

int RegionSession::newPayload(const std::string& uuid,uint64_t timeout) {
	do {
		if( nullptr == m_req )
			break;
		if( uuid.empty() )
			break;
		region::api::AddPayload msg;
		msg.set_uuid(uuid);
		msg.set_rep( timeout != 0 );
		if( -1 == zpb_send(m_req,msg,true) )
			break;

		if( 0 == timeout ) {
			return 0;
		}

		if( zmq_wait_readable(m_req,timeout) <= 0 )
			break;
		region::api::Result result;
		if( -1 == zpb_recv(result,m_req) )
			break;
		return result.result();
	} while(0);
	return -1;
}

int RegionSession::rmPayload(const std::string& uuid,uint64_t timeout) {
	do {
		if( nullptr == m_req )
			break;
		if( uuid.empty() )
			break;
		region::api::RmPayload msg;
		msg.set_uuid(uuid);
		msg.set_rep( timeout != 0 );
		if( -1 == zpb_send(m_req,msg,true) )
			break;

		if( 0 == timeout ) {
			return 0;
		}

		if( zmq_wait_readable(m_req,timeout) <= 0 )
			break;
		region::api::Result result;
		if( -1 == zpb_recv(result,m_req) )
			break;
		return result.result();
	} while(0);
	return -1;
}

int RegionSession::newService(const std::string& name,const std::string& address,uint64_t timeout) {
	do {
		if( nullptr == m_req )
			break;
		if( name.empty() || address.empty() )
			break;
		region::api::AddService msg;
		msg.set_name(name);
		msg.set_address(address);
		msg.set_rep( timeout != 0 );
		if( -1 == zpb_send(m_req,msg,true) )
			break;

		if( 0 == timeout ) {
			return 0;
		}
		if( zmq_wait_readable(m_req,timeout) <= 0 )
			break;
		region::api::Result result;
		if( -1 == zpb_recv(result,m_req) )
			break;
		return result.result();
	} while(0);
	return -1;
}

int RegionSession::rmService(const std::string& name,uint64_t timeout) {
	do {
		if( nullptr == m_req )
			break;
		if( name.empty() )
			break;
		region::api::RmService msg;
		msg.set_name(name);
		msg.set_rep( timeout != 0 );
		if( -1 == zpb_send(m_req,msg,true) )
			break;
		if( 0 == timeout ) {
			return 0;
		}
		if( zmq_wait_readable(m_req,timeout) <= 0 )
			break;
		region::api::Result result;
		if( -1 == zpb_recv(result,m_req) )
			break;
		return result.result();
	} while(0);
	return -1;
}


