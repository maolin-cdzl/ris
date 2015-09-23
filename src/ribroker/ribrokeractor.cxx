#include <glog/logging.h>
#include "ribroker/ribrokeractor.h"

void ribroker_start(void* ctx,const char* front_addr,const char* back_addr);

RIBrokerActor::RIBrokerActor() :
	m_thread(0),
	m_ctx(nullptr)
{
}

RIBrokerActor::~RIBrokerActor() {
	stop();
}

int RIBrokerActor::start(const std::string& frontend,const std::string& backend) {
	if( m_thread != 0 )
		return -1;
	CHECK(nullptr == m_ctx);

	m_ctx = zmq_ctx_new();
	m_front_addr = frontend;
	m_back_addr = backend;
	
	pthread_create(&m_thread,NULL,&RIBrokerActor::broker_routine,this);
	return 0;
}

void RIBrokerActor::stop() {
	if( m_thread != 0 ) {
		CHECK_NOTNULL(m_ctx);
		zmq_ctx_term(m_ctx);
		void* res = nullptr;
		pthread_join(m_thread,&res);
		m_thread = 0;
		zmq_ctx_destroy(m_ctx);
		m_ctx = nullptr;
	}
}

void* RIBrokerActor::broker_routine(void* arg) {
	RIBrokerActor* self = (RIBrokerActor*)arg;
	ribroker_start(self->m_ctx,self->m_front_addr.c_str(),self->m_back_addr.c_str());
	return nullptr;
}

