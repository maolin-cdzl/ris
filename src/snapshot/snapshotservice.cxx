#include <glog/logging.h>
#include "ris/snapshot/snapshotservice.h"
#include "ris/snapshot.pb.h"
#include "zmqx/zprotobuf++.h"


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
	if( m_sock != nullptr ) {
		return -1;
	}

	do {
		m_sock = zsock_new(ZMQ_REP);
		if( -1 == zsock_bind(m_sock,"%s",m_svc_address.c_str()) ) {
			LOG(FATAL) << "SnapshotService can NOT bind to: " << m_svc_address;
			break;
		}

		if( -1 == zloop_reader(loop,m_sock,mainReaderAdapter,this) ) {
			LOG(FATAL) << "SnapshotService register reader failed";
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
	snapshot::SnapshotRep rep;
	rep.set_result(-1);

	do {
		snapshot::SnapshotReq req;
		if( -1 == zpb_recv(req,m_sock) ) {
			LOG(ERROR) << "SnapshotService recv SnapshotReq error";
			break;
		}

		LOG(INFO) << "SnapshotService recv request";
		
		if( m_workers.size() >= m_capacity ) {
			LOG(ERROR) << "SnapshotService busy,current worker " << m_workers.size();
			break;
		}
		auto snapshot = m_snapshotable->buildSnapshot();
		if( snapshot.empty() ) {
			LOG(ERROR) << "SnapshotService build snapshot faile";
			break;
		}

		std::shared_ptr<SnapshotServiceWorker> worker(new SnapshotServiceWorker(m_worker_address));
		if( 0 == worker->start(snapshot) ) {
			auto endpoint = worker->endpoint();
			m_workers.push_back(worker);
			zloop_reader(loop,zactor_sock(worker->actor()),workerReaderAdapter,this);

			rep.set_result(0);
			rep.set_address(endpoint);
			LOG(INFO) << "SnapshotService start transform snapshot with " << snapshot.size() << " item";
		} else {
			LOG(ERROR) << "SnapshotService start worker failed";
		}
	} while(0);

	zpb_send(m_sock,rep);
	return 0;
}

int SnapshotService::onWorkerReadable(zloop_t* loop,zsock_t* reader) {
	auto worker = findWorker(reader);
	if( worker == nullptr ) {
		LOG(FATAL) << "SnapshotService can NOT found worker in onWorkerReadable";
		return -1;
	}

	zmsg_t* msg = zmsg_recv(worker->actor());
	if( msg ) {
		char* result = zframe_strdup( zmsg_first(msg) );
		LOG(INFO) << "SnapshotServiceWorker done with result: " << result;
		free(result);
		zmsg_destroy(&msg);
	}

	zloop_reader_end(loop,reader);
	for(auto it = m_workers.begin(); it != m_workers.end(); ++it) {
		if( (*it) == worker ) {
			m_workers.erase(it);
			return 0;
		}
	}
	LOG(FATAL) << "some SnapsthoServiceWorker say it's done,but can not found in m_workers";
	return -1;
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
