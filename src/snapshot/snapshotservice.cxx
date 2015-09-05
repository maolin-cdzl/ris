#include <glog/logging.h>
#include "snapshot/snapshotservice.h"
#include "ris/snapshot.pb.h"
#include "zmqx/zprotobuf++.h"


SnapshotService::SnapshotService(zloop_t* loop) :
	m_loop(loop),
	m_capacity(4),
	m_rep_reader(loop)
{
}


SnapshotService::~SnapshotService() {
	stop();
}

int SnapshotService::start(const std::shared_ptr<ISnapshotable>& snapshotable,const std::string& svcAddress,const std::string& workerAddress,size_t capacity) {
	if( m_rep_reader.isActive() )
		return -1;
	m_snapshotable = snapshotable;
	m_svc_address = svcAddress;
	m_worker_address = workerAddress;
	m_capacity = capacity;

	LOG(INFO) << "SnapshotService start listen on: " << svcAddress << " , limit " << capacity << " works address to: " << workerAddress;

	zsock_t* rep = nullptr;
	do {
		rep = zsock_new(ZMQ_REP);
		if( -1 == zsock_bind(rep,"%s",m_svc_address.c_str()) ) {
			LOG(FATAL) << "SnapshotService can NOT bind to: " << m_svc_address;
			break;
		}

		if( -1 == m_rep_reader.start(&rep,std::bind<int>(&SnapshotService::onRepReadable,this,std::placeholders::_1)) ) {
			LOG(FATAL) << "SnapshotService start reader failed";
			break;
		}
		return 0;
	} while( 0 );

	if( rep ) {
		zsock_destroy(&rep);
	}
	return -1;
}

int SnapshotService::stop() {
	if( m_rep_reader.isActive() ) {
		LOG(INFO) << "SnapshotService stop";
		m_rep_reader.stop();
		while(! m_workers.empty() ) {
			auto it = m_workers.front();
			zloop_reader_end(m_loop,zactor_sock(it->actor()));
			m_workers.pop_front();
		}
		return 0;
	} else {
		return -1;
	}
}

int SnapshotService::onRepReadable(zsock_t* reader) {
	snapshot::SnapshotRep rep;
	rep.set_result(-1);

	do {
		snapshot::SnapshotReq req;
		if( -1 == zpb_recv(req,reader) ) {
			LOG(ERROR) << "SnapshotService recv SnapshotReq error";
			break;
		}
		
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
		LOG(INFO) << "SnapshotService start transform snapshot with " << snapshot.size() << " item";
		if( 0 == worker->start(snapshot) ) {
			auto endpoint = worker->endpoint();
			m_workers.push_back(worker);
			zloop_reader(m_loop,zactor_sock(worker->actor()),workerReaderAdapter,this);

			rep.set_result(0);
			rep.set_address(endpoint);
		} else {
			LOG(ERROR) << "SnapshotService start worker failed";
		}
	} while(0);

	zpb_send(reader,rep);
	return 0;
}

int SnapshotService::onWorkerReadable(zloop_t* loop,zsock_t* reader) {
	auto worker = popWorker(reader);
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

	return 0;
}

std::shared_ptr<SnapshotServiceWorker> SnapshotService::popWorker(zsock_t* sock) {
	for(auto it=m_workers.begin(); it != m_workers.end(); ++it) {
		if( zactor_sock((*it)->actor()) == sock ) {
			auto p = *it;
			m_workers.erase(it);
			zloop_reader_end(m_loop,sock);
			return p;
		}
	}
	return std::shared_ptr<SnapshotServiceWorker>(nullptr);
}

/**
 * adapter method
 */

int SnapshotService::workerReaderAdapter(zloop_t* loop,zsock_t* reader,void* arg) {
	assert(loop);
	assert(reader);
	assert(arg);

	SnapshotService* self = (SnapshotService*)arg;
	return self->onWorkerReadable(loop,reader);
}
