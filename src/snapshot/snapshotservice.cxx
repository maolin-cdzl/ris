#include <glog/logging.h>
#include "snapshot/snapshotservice.h"
#include "ris/snapshot.pb.h"
#include "zmqx/zprotobuf++.h"


SnapshotService::SnapshotService() :
	m_running(false),
	m_actor(nullptr),
	m_capacity(0),
	m_period_count(0),
	m_tv_timeout(0)
{
}


SnapshotService::~SnapshotService() {
	stop();
}

int SnapshotService::start(const std::shared_ptr<ISnapshotable>& snapshotable,const std::string& address,size_t capacity,size_t period_count,ri_time_t timeout) {
	if( nullptr != m_actor )
		return -1;

	m_snapshotable = snapshotable;
	m_address = address;
	m_capacity = capacity;
	m_period_count = period_count;
	m_tv_timeout = timeout;

	LOG(INFO) << "SnapshotService start listen on: " << address << ", capacity " << capacity << ", period count " << period_count << ", timeout " << timeout;

	m_actor = zactor_new(&SnapshotService::actorAdapter,this);
	if( m_actor ) {
		return 0;
	} else {
		return -1;
	}
}

void SnapshotService::stop() {
	m_running = false;
	if( m_actor ) {
		zactor_destroy(&m_actor);
	}
}

void SnapshotService::actorAdapter(zsock_t* pipe,void* arg) {
	SnapshotService* self = (SnapshotService*)arg;
	self->run(pipe);
}

void SnapshotService::run(zsock_t* pipe) {
	m_running = true;
	zsock_signal(pipe,0);

	zloop_t* loop = nullptr;
	zsock_t* router = nullptr;
	
	do {
		loop = zloop_new();
		CHECK_NOTNULL(loop);

		router = zsock_new(ZMQ_ROUTER);
		CHECK_NOTNULL(router);

		if( -1 == zsock_bind(router,"%s",m_address.c_str()) ) {
			LOG(FATAL) << "SnapshotService can not bind to " << m_address;
			break;
		}

		auto zdisp = std::make_shared<ZDispatcher>(loop);
		if( -1 == zdisp->start(&router,make_dispatcher(*zdisp),true) ) {
			LOG(FATAL) << "SnapshotService can not start dispatcher";
			break;
		}

		auto reader = std::make_shared<ZLoopReader>(loop);
		if( -1 == reader->start(pipe,std::bind<int>(&SnapshotService::onPipeReadable,this,std::placeholders::_1)) ) {
			LOG(FATAL) << "SnapshotService can not start pipe reader";
			break;
		}

		auto timer = std::make_shared<ZLoopTimer>(loop);
		if( -1 == timer->start(1000,0,std::bind<int>(&SnapshotService::onTimer,this)) ) {
			LOG(FATAL) << "SnapshotService can not start timer";
			break;
		}

		while( m_running ) {
			if( 0 == zloop_start(loop) ) {
				m_running = false;
				break;
			}
		}
	} while(0);

	m_running = false;
	if( router ) {
		zsock_destroy(&router);
	}
	if( loop ) {
		zloop_destroy(&loop);
	}
}

std::shared_ptr<Dispatcher> SnapshotService::make_dispatcher(ZDispatcher& zdisp) {
	auto disp = std::make_shared<Dispatcher>();
	disp->register_processer(snapshot::SnapshotReq::descriptor(),std::bind<int>(&SnapshotService::onSnapshotReq,this,std::ref(zdisp),std::placeholders::_1));
	disp->register_processer(snapshot::SyncSignalRep::descriptor(),std::bind<int>(&SnapshotService::onSyncSignal,this,std::ref(zdisp),std::placeholders::_1));
	return std::move(disp);
}

int SnapshotService::onSnapshotReq(ZDispatcher& zdisp,const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<snapshot::SnapshotReq>(msg);
	CHECK(p);

	snapshot::SnapshotRep rep;
	auto it = m_workers.find(p->uuid());
	if( it != m_workers.end() ) {
		LOG(WARNING) << "Client repeated send request while sync is processing: " << p->uuid();
		rep.set_result(-1);
		zdisp.sendback(rep);
	} else if( m_workers.size() < m_capacity ) {
		LOG(INFO) << "Accept client snapshot request: " << p->uuid();
		rep.set_result(0);
		zdisp.shadow_sendback(rep);

		auto worker = std::make_shared<SnapshotServiceWorker>(m_snapshotable->buildSnapshot());
		if( worker->sendItems(zdisp.prepend(),zdisp.socket(),m_period_count) > 0 ) {
			m_workers.insert( std::make_pair(p->uuid(),worker) );
			snapshot::SyncSignalReq sync;
			zdisp.sendback(sync);
		}
		
	} else {
		LOG(WARNING) << "Too many client ask for service";
		rep.set_result(-1);
		zdisp.sendback(rep);
	}

	return 0;
}

int SnapshotService::onSyncSignal(ZDispatcher& zdisp,const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<snapshot::SyncSignalRep>(msg);
	CHECK(p);
	auto it = m_workers.find(p->uuid());
	if( it != m_workers.end() ) {
		if( it->second->sendItems(zdisp.prepend(),zdisp.socket(),m_period_count) == 0 ) {
			m_workers.erase(it);
		}
	}
	return 0;
}



int SnapshotService::onPipeReadable(zsock_t* reader) {
	zmsg_t* msg = zmsg_recv(reader);

	if( msg ) {
		zmsg_destroy(&msg);
	}
	return -1;
}

int SnapshotService::onTimer() {
	const ri_time_t now = ri_time_now();
	for(auto it=m_workers.begin(); it != m_workers.end();) {
		if( now - it->second->lastSend() > m_tv_timeout ) {
			it = m_workers.erase(it);
		} else {
			++it;
		}
	}
	return 0;
}

