#include "ris/snapshot/snapshotservice.h"


SnapshotService::SnapshotService(zloop_t* loop) :
	m_loop(loop),
	m_capacity(4),
	m_sock(nullptr)
{
}


SnapshotService::~SnapshotService() {
	stop();
}

int SnapshotService::start(const std::shared_ptr<ISnapshotable>& snapshotable,const std::string& svcAddress,const std::string& workerAddress,size_t capacity) {
	m_snapshotable = snapshotable;
	m_svc_address = svcAddress;
	m_worker_address = workerAddress;
	m_capacity = capacity;

	return startLoop(m_loop);
}

int SnapshotService::stop() {
	if( m_sock ) {
		stopLoop(m_loop);
		return 0;
	} else {
		return -1;
	}
}

int SnapshotService::startLoop(zloop_t* loop) {

	do {
		if( m_sock != nullptr ) {
			break;
		}

		m_sock = zsock_new(ZMQ_REP);
		if( -1 == zsock_bind(m_sock,"%s",m_svc_address.c_str()) ) {
			break;
		}

		if( -1 == zloop_reader(loop,m_sock,mainReaderAdapter,this) ) {
			break;
		}
		return 0;
	} while( 0 );

	if( m_sock ) {
		zsock_destroy(&m_sock);
	}
	return -1;
}

void SnapshotService::stopLoop(zloop_t* loop) {
	while(! m_workers.empty() ) {
		auto it = m_workers.front();
		zloop_reader_end(loop,zactor_sock(it->actor()));
		m_workers.pop_front();
	}
	
	zloop_reader_end(loop,m_sock);
	zsock_destroy(&m_sock);
}

int SnapshotService::onMainReadable(zloop_t* loop) {
	zmsg_t* msg = nullptr;
	bool good = false;

	do {
		msg = zmsg_recv(m_sock);
		if( msg == nullptr )
			break;
		
		zframe_t* fr = zmsg_first(msg);
		if( fr == nullptr )
			break;
		if( zframe_streq(fr,"$ssreq") ) {
			if( m_workers.size() >= m_capacity )
				break;
			std::shared_ptr<SnapshotServiceWorker> worker(new SnapshotServiceWorker(m_worker_address));
			auto snapshot = m_snapshotable->buildSnapshot();
			if( 0 == worker->start(snapshot) ) {
				auto endpoint = worker->endpoint();
				m_workers.push_back(worker);
				
				zloop_reader(loop,zactor_sock(worker->actor()),workerReaderAdapter,this);
				zstr_sendm(m_sock,"ok");
				zstr_send(m_sock,endpoint.c_str());
				good = true;
			}
		} else {
			break;
		}
		
	} while(0);

	if( msg ) {
		zmsg_destroy(&msg);
	}

	if( ! good ) {
		zstr_send(m_sock,"error");
	}
	return 0;
}

int SnapshotService::onWorkerReadable(zloop_t* loop,zsock_t* reader) {
	auto worker = findWorker(reader);
	if( worker == nullptr ) {
		// log it
		return -1;
	}

	zmsg_t* msg = zmsg_recv(worker->actor());
	if( msg ) {
		// log it
		zmsg_destroy(&msg);
	}

	zloop_reader_end(loop,reader);
	for(auto it = m_workers.begin(); it != m_workers.end(); ++it) {
		if( (*it) == worker ) {
			m_workers.erase(it);
			break;
		}
	}
	return 0;
}

std::shared_ptr<SnapshotServiceWorker> SnapshotService::findWorker(zsock_t* sock) {
	for(auto it=m_workers.begin(); it != m_workers.end(); ++it) {
		if( zactor_sock((*it)->actor()) == sock )
			return (*it);
	}
	return std::shared_ptr<SnapshotServiceWorker>(nullptr);
}

/**
 * adapter method
 */

int SnapshotService::mainReaderAdapter(zloop_t* loop,zsock_t* reader,void* arg) {
	assert(loop);
	assert(reader);
	assert(arg);

	SnapshotService* self = (SnapshotService*)arg;
	assert(self->m_sock == reader);
	return self->onMainReadable(loop);
}

int SnapshotService::workerReaderAdapter(zloop_t* loop,zsock_t* reader,void* arg) {
	assert(loop);
	assert(reader);
	assert(arg);

	SnapshotService* self = (SnapshotService*)arg;
	return self->onWorkerReadable(loop,reader);
}
