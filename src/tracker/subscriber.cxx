#include <glog/logging.h>
#include "ris/tracker/subscriber.h"
#include "zmqx/zprotobuf++.h"


RISubscriber::RISubscriber(zloop_t* loop) :
	m_loop(loop),
	m_sub(nullptr),
	m_disp(new ZDispatcher(loop))
{
	auto disp = std::make_shared<Dispatcher>();
}

RISubscriber::~RISubscriber() {
	stop();
}


int RISubscriber::start(const std::string& address,const std::shared_ptr<IRIObserver>& ob) {
	int result = -1;
	assert( ! brokers.empty() );
	assert( ob );

	do {
		if( m_sub )
			break;
		m_observer = ob;
		m_sub = zsock_new(ZMQ_SUB);

		if( -1 == zsock_connect(m_sub,"%s",address.c_str()) ) {
			LOG(ERROR) << "Subscriber can NOT connect to: " << address;
			break;
		}

		if( -1 == zloop_reader(m_loop,m_sub,subReadableAdapter,this) )
			break;
		result = 0;
	} while( 0 );

	if( -1 == result ) {
		if( m_sub ) {
			zsock_destroy(&m_sub);
		}
		if( m_observer ) {
			m_observer.reset();
		}
	}
	return result;
}

int RISubscriber::stop() {
	if( m_sub == nullptr )
		return -1;

	zloop_reader_end(m_loop,m_sub);
	zsock_destroy(&m_sub);
	return 0;
}

int RISubscriber::onSubReadable() {
	auto msg = zpb_recv(m_sub);
	if( msg ) {
	}
}

int RISubscriber::subReadableAdapter(zloop_t* loop,zsock_t* reader,void* arg) {
	(void)loop;
	(void)reader;
	RISubscriber* self = (RISubscriber*)arg;
	return self->onSubReadable();
}

