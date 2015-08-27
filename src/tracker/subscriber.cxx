#include "ris/tracker/subscriber.h"


RISubscriber::RISubscriber(zloop_t* loop) :
	m_loop(loop),
	m_sub(nullptr)
{
}

RISubscriber::~RISubscriber() {
	stop();
}

static int connectBrokers(zsock_t* sock,const std::list<std::string>& brokers);

int RISubscriber::start(const std::list<std::string>& brokers,const std::shared_ptr<IRIObserver>& ob) {
	int result = -1;
	assert( ! brokers.empty() );
	assert( ob );

	do {
		if( m_sub )
			break;
		m_observer = ob;
		m_sub = zsock_new(ZMQ_SUB);

		if( -1 == connectBrokers(m_sub,brokers) )
			break;

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
}

int RISubscriber::subReadableAdapter(zloop_t* loop,zsock_t* reader,void* arg) {
	(void)loop;
	(void)reader;
	RISubscriber* self = (RISubscriber*)arg;
	return self->onSubReadable();
}

static int connectBrokers(zsock_t* sock,const std::list<std::string>& brokers) {
	int result = 0;
	for(auto it=brokers.begin(); it != brokers.end(); ++it) {
		if( -1 == zsock_connect(sock,"%s",it->c_str()) ) {
			result = -1;
			break;
		}
	}
	return result;
}

