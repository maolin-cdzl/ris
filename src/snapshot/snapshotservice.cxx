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

	zloop_t* loop = nullptr;
	zsock_t* router = nullptr;
	
	LOG(INFO) << "SnapshotService starting...";
	do {
		loop = zloop_new();
		CHECK_NOTNULL(loop);

		router = zsock_new(ZMQ_ROUTER);
		CHECK_NOTNULL(router);
		zsock_set_identity(router,new_short_identity().c_str());
#ifndef NDEBUG
		zsock_set_router_mandatory(router,1);
#endif

		if( (size_t) zsock_sndhwm(router) < m_capacity * m_period_count * 2 ) {
			LOG(INFO) << "Set router socket send hwm from " << zsock_sndhwm(router) << " to " << m_capacity * m_period_count * 2;
			zsock_set_sndhwm(router,m_capacity * m_period_count * 2);
		}

		if( -1 == zsock_bind(router,"%s",m_address.c_str()) ) {
			LOG(FATAL) << "SnapshotService can not bind to " << m_address;
			break;
		}

		auto router_reader = make_zpb_reader(loop,&router,make_dispatcher());
		CHECK(router_reader) << "SnapshotService can not start router reader";

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

		LOG(INFO) << "SnapshotService started";
		zsock_signal(pipe,0);
		while( m_running ) {
			if( 0 == zloop_start(loop) ) {
				LOG(INFO) << "ZMQ interrupted";
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
	LOG(INFO) << "SnapshotService shutdown";
}

std::shared_ptr<envelope_dispatcher_t> SnapshotService::make_dispatcher() {
	auto disp = std::make_shared<envelope_dispatcher_t>();
	disp->register_processer(snapshot::SnapshotReq::descriptor(),std::bind<int>(&SnapshotService::onSnapshotReq,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
	disp->register_processer(snapshot::SyncSignalRep::descriptor(),std::bind<int>(&SnapshotService::onSyncSignal,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
	return std::move(disp);
}

int SnapshotService::onSnapshotReq(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,const std::shared_ptr<ZEnvelope>& envelope) {
	CHECK(envelope);
	auto p = std::dynamic_pointer_cast<snapshot::SnapshotReq>(msg);
	CHECK(p);

	snapshot::SnapshotRep rep;
	auto it = m_workers.find(p->requester());
	if( it != m_workers.end() ) {
		LOG(WARNING) << "Client repeated send request while sync is processing: " << p->requester();
		rep.set_result(-1);
		zpb_send(sock,envelope,rep);
	} else if( m_workers.size() < m_capacity ) {
		rep.set_result(0);
		if( 0 == zpb_send(sock,envelope,rep) ) {
			LOG(INFO) << "Accept client snapshot request: " << p->requester();

			auto worker = std::make_shared<SnapshotServiceWorker>(m_snapshotable->buildSnapshot());
			const size_t left = worker->sendItems(sock,envelope,m_period_count);
			if( left == 0 ) {
				LOG(INFO) << "Send all snapshot item to client done. " << p->requester();
			} else {
				LOG(INFO) << "Send part items to client " << p->requester() << " " << m_period_count << "/" << left;
				m_workers.insert( std::make_pair(p->requester(),worker) );
				snapshot::SyncSignalReq sync;
				zpb_send(sock,envelope,sync);
			}
		}
	} else {
		LOG(WARNING) << "Too many client ask for service";
		rep.set_result(-1);
		zpb_send(sock,envelope,rep);
	}

	return 0;
}

int SnapshotService::onSyncSignal(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,const std::shared_ptr<ZEnvelope>& envelope) {
	auto p = std::dynamic_pointer_cast<snapshot::SyncSignalRep>(msg);
	CHECK(p);
	auto it = m_workers.find(p->requester());
	if( it != m_workers.end() ) {
		const size_t left = it->second->sendItems(sock,envelope,m_period_count);
		if( left == 0 ) {
			LOG(INFO) << "Send all snapshot item to client done. " << p->requester();
			m_workers.erase(it);
		} else {
			LOG(INFO) << "Send part items to client " << p->requester() << " " << m_period_count << "/" << left;
			snapshot::SyncSignalReq sync;
			zpb_send(sock,envelope,sync);
		}
	} else {
		LOG(WARNING) << "Recv unknown client id: " << p->requester();
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
			LOG(WARNING) << "Client timeout: " << it->first;
			it = m_workers.erase(it);
		} else {
			++it;
		}
	}
	return 0;
}

