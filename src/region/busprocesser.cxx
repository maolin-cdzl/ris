#include <glog/logging.h>
#include "region/busprocesser.h"
#include "zmqx/zhelper.h"

#define WORKER_READY		"\001"

BusProcesser::BusProcesser(zloop_t* loop) :
	m_loop(loop),
	m_hwm(5000),
	m_bus_reader(nullptr),
	m_worker_reader(nullptr)
{
}

BusProcesser::~BusProcesser() {
	stop();
}

int BusProcesser::start(const std::shared_ptr<RIRegionTable>& table,const std::shared_ptr<RegionCtx>& ctx) {
	if( m_bus_reader || m_worker_reader )
		return -1;

	CHECK(table);
	CHECK_GT(ctx->bus_hwm,1);

	zsock_t* busend = nullptr;
	zsock_t* workerend = nullptr;
	do {
		busend = zsock_new(ZMQ_ROUTER);
		CHECK_NOTNULL(busend);
		zsock_set_identity(busend,ctx->bus_identity.c_str());
		if( -1 == zsock_bind(busend,"%s",ctx->bus_address.c_str()) ) {
			LOG(FATAL) << "Busend socket can not bind to: " << ctx->bus_address;
			break;
		}

		m_bus_reader = std::make_shared<ZLoopReader>(m_loop);
		CHECK(m_bus_reader);
		if( -1 == m_bus_reader->start(&busend,std::bind<int>(&BusProcesser::onBusReadable,this,std::placeholders::_1)) ) {
			LOG(FATAL) << "Can not start bus reader";
			break;
		}

		workerend = zsock_new(ZMQ_ROUTER);
		CHECK_NOTNULL(workerend);
		zsock_set_identity(workerend,ctx->worker_identity.c_str());
		zsock_set_router_mandatory(workerend,1);
		if( -1 == zsock_bind(workerend,"%s",ctx->worker_address.c_str()) ) {
			LOG(FATAL) << "Workerend socket can not bind to: " << ctx->worker_address;
			break;
		}
		m_worker_reader = std::make_shared<ZLoopReader>(m_loop);
		CHECK(m_worker_reader);
		if( -1 == m_worker_reader->start(&workerend,std::bind<int>(&BusProcesser::onWorkerReadable,this,std::placeholders::_1))) {
			LOG(FATAL) << "Can not start worker reader";
		}
		m_table = table;
		m_hwm = ctx->bus_hwm;
		return 0;
	} while(0);

	stop();
	if( busend ) {
		zsock_destroy(&busend);
	}
	if( workerend ) {
		zsock_destroy(&workerend);
	}
	return -1;
}

void BusProcesser::stop() {
	if( m_worker_reader ) {
		m_worker_reader.reset();
	}
	if( m_bus_reader ) {
		m_bus_reader.reset();
	}

	for(auto it = m_msgs.begin(); it != m_msgs.end(); ++it) {
		zmsg_t* msg = *it;
		zmsg_destroy(&msg);
	}
	m_msgs.clear();
	m_workers_index.clear();
	m_workers.clear();
	m_table.reset();
}

int BusProcesser::onBusReadable(zsock_t* sock) {
	zmsg_t* msg = zmsg_recv(sock);
	sendto_worker(&msg);
	if( msg  ) {
		m_msgs.push_back(msg);
		if( m_msgs.size() > m_hwm ) {
			zmsg_t* old = m_msgs.front();
			m_msgs.pop_front();
			zmsg_destroy(&old);
		}
	}
	return 0;
}

int BusProcesser::onWorkerReadable(zsock_t* sock) {
	zmsg_t* msg = nullptr;

	do {
		msg = zmsg_recv(sock);
		if( nullptr == msg ) {
			break;
		}

		const std::string worker = zmq_pop_router_identity(msg);
		if( worker.empty() ) {
			break;
		}
		push_worker(worker);

		zframe_t* fr = zmsg_first(msg);
		if( memcmp(zframe_data(fr),WORKER_READY,1) == 0 ) {
			DLOG(INFO) << "Worker ready: " << worker;
		} else {
			//forward message to broker if it's not a READY.
			DLOG(INFO) << "Forward worker's reply message and mark it READY: " << worker;
			zmsg_send(&msg,m_bus_reader->socket());
		}
	} while(0);

	if( msg ) {
		zmsg_destroy(&msg);
	}

	return 0;
}

int BusProcesser::sendto_worker(zmsg_t** p_msg) {
	while( *p_msg && ! m_workers.empty() ) {
		std::string worker = m_workers.front();

		if( -1 == zstr_sendm(m_worker_reader->socket(),worker.c_str()) ) {
			LOG(WARNING) << "Worker is invalid: " << worker;
			pop_worker();
			continue;
		}
		if( -1 == zmsg_send(p_msg,m_worker_reader->socket()) ) {
			// this error is not upon worker invalid,so don't keep trying.
			LOG(WARNING) << "Send to worker failed: " << worker;
			break;
		}
		pop_worker();
		return 0;
	}
	return -1;
}

void BusProcesser::push_worker(const std::string& worker) {
	auto it = m_workers_index.find(worker);
	if( it == m_workers_index.end() ) {
		auto itc = m_workers.insert(m_workers.end(),worker);
		m_workers_index[worker] = itc;
	} else {
		auto itc = it->second;
		m_workers.splice(m_workers.end(),m_workers,itc);
	}
}

void BusProcesser::pop_worker() {
	if( ! m_workers.empty() ) {
		auto it = m_workers_index.find(m_workers.front());
		CHECK( m_workers_index.end() != it );
		m_workers_index.erase(it);
		m_workers.pop_front();
	}
}

