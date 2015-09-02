#include <glog/logging.h>
#include "snapshot/snapshotclient.h"
#include "ris/snapshot.pb.h"
#include "zmqx/zhelper.h"
#include "zmqx/zprotobuf++.h"


SnapshotClient::SnapshotClient(zloop_t* loop) :
	m_loop(loop),
	m_sock(nullptr),
	m_tid(-1),
	m_tv_timeout(0)
{
}

SnapshotClient::~SnapshotClient() {
	stop();
}

int SnapshotClient::start(const std::function<void(int)>& ob,const std::shared_ptr<ISnapshotBuilder>& builder,const std::string& address) {
	if( m_sock != nullptr )
		return -1;
	
	do {
		m_sock = zsock_new_req(address.c_str());
		if( nullptr == m_sock ) {
			LOG(ERROR) << "SnapshotClient can NOT connect to: " << address;
			break;
		}

		snapshot::SnapshotReq req;
		if( -1 == zpb_send(m_sock,req) ) {
			LOG(ERROR) << "SnapshotClient send SnapshotReq failed";
			break;
		}

		m_fn_readable = std::bind<int>(&SnapshotClient::onReqReadable,this,std::placeholders::_1);
		
		if( -1 == zloop_reader(m_loop,m_sock,readableAdapter,this) )
			break;
		m_tid = zloop_timer(m_loop,500,0,timerAdapter,this);
		if( m_tid == -1 )
			break;
		m_tv_timeout = ri_time_now() + 1000;
		m_builder = builder;
		m_observer = ob;
		m_last_region.clear();
		return 0;
	} while( 0 );

	stop();
	return -1;
}

void SnapshotClient::stop() {
	if( m_tid != -1 ) {
		zloop_timer_end(m_loop,m_tid);
		m_tid = -1;
	}
	if( m_sock ) {
		zloop_reader_end(m_loop,m_sock);
		zsock_destroy(&m_sock);
	}
	m_fn_readable = nullptr;
	m_observer = nullptr;
	m_builder.reset();
	m_last_region.clear();
}

int SnapshotClient::pullSnapshotBegin(zsock_t* sock) {
	do {
		snapshot::SnapshotBegin msg;
		if( -1 == zpb_recv(msg,sock) ) {
			LOG(ERROR) << "SnapshotClient recv SnapshotBegin failed";
			break;
		}

		m_fn_readable = std::bind<int>(&SnapshotClient::pullRegionOrFinish,this,std::placeholders::_1);
		m_tv_timeout = ri_time_now() + 1000;
		return 0;
	} while(0);

	auto ob = m_observer;
	stop();
	ob(SNAPSHOT_CLIENT_ERROR);
	return -1;
}

int SnapshotClient::pullRegionOrFinish(zsock_t* sock) {
	assert( m_last_region.empty() );

	do {
		auto msg = zpb_recv(sock);
		if( msg == nullptr ) {
			LOG(ERROR) << "SnapshotClient recv RegionBegin or SnapshotEnd failed";
			break;
		}
		if( msg->GetDescriptor() == snapshot::RegionBegin::descriptor() ) {
			auto p = std::dynamic_pointer_cast<snapshot::RegionBegin>(msg);
			Region region;
			region.id = p->uuid();
			region.version = p->version();
			if( p->has_idc() ) {
				region.idc = p->idc();
			}
			if( p->has_bus_address() ) {
				region.bus_address = p->bus_address();
			}
			if( p->has_snapshot_address() ) {
				region.snapshot_address = p->snapshot_address();
			}

			const ri_time_t now = ri_time_now();
			region.timeval = now;

			DLOG(INFO) << "SnapshotClient recv RegionBegin: " << region.id << "(" << region.version << ")"; 
			if( -1 == m_builder->addRegion(region) ) {
				LOG(ERROR) << "SnapshotClient addRegion failed,region: " <<  region.id << "(" << region.version << ")";
				break;
			}
			m_last_region = p->uuid();
			m_fn_readable = std::bind<int>(&SnapshotClient::pullRegionBody,this,std::placeholders::_1);
			m_tv_timeout = now + 1000;
		} else if( msg->GetDescriptor() == snapshot::SnapshotEnd::descriptor() ) {
			DLOG(INFO) << "SnapshotClient recv SnapshotEnd"; 
			auto ob = m_observer;
			stop();
			ob(0);
		} else {
			LOG(ERROR) << "SnapshotClient recv unexpect message: " << msg->GetTypeName() << ", when wait RegionBegin or SnapshotEnd";
		}
		return 0;
	} while(0);

	auto ob = m_observer;
	stop();
	ob(SNAPSHOT_CLIENT_ERROR);
	return -1;
}

int SnapshotClient::pullRegionBody(zsock_t* sock) {
	assert( ! m_last_region.empty() );

	do {
		auto msg = zpb_recv(sock);
		if( msg == nullptr ) {
			LOG(ERROR) << "SnapshotClient recv Service or Payload failed";
			break;
		}
		const ri_time_t now = ri_time_now();
		if( msg->GetDescriptor() == snapshot::Payload::descriptor() ) {
			auto p = std::dynamic_pointer_cast<snapshot::Payload>(msg);
			Payload pl;
			pl.id = p->uuid();
			pl.timeval = now;

			DLOG(INFO) << "SnapshotClient recv Payload: " << pl.id; 
			if( -1 == m_builder->addPayload(m_last_region,pl) ) {
				LOG(ERROR) << "Snapshot addPayload failed: " << pl.id;
				break;
			}
			m_tv_timeout = now + 1000;
		} else if( msg->GetDescriptor() == snapshot::Service::descriptor() ) {
			auto p = std::dynamic_pointer_cast<snapshot::Service>(msg);
			Service svc;
			svc.name = p->name();
			svc.address = p->address();
			svc.timeval = ri_time_now();

			DLOG(INFO) << "SnapshotClient recv Service: " << svc.name; 
			if( -1 == m_builder->addService(m_last_region,svc) ) {
				LOG(ERROR) << "Snapshot addService failed: " << svc.name;
				break;
			}
			m_tv_timeout = now + 1000;
		} else if( msg->GetDescriptor() == snapshot::RegionEnd::descriptor() ) {
			DLOG(INFO) << "SnapshotClient recv RegionEnd: ";
			m_last_region.clear();
			m_fn_readable = std::bind<int>(&SnapshotClient::pullRegionOrFinish,this,std::placeholders::_1);
			m_tv_timeout = now + 1000;
			return 0;
		} else {
			LOG(ERROR) << "SnapshotClient pullRegionBody recv unexpect message: " << msg->GetTypeName();
			break;
		}
	} while(1);

	auto ob = m_observer;
	stop();
	ob(SNAPSHOT_CLIENT_ERROR);
	return -1;
}

int SnapshotClient::readableAdapter(zloop_t* loop,zsock_t* reader,void* arg) {
	(void)loop; (void) reader;
	SnapshotClient* self = (SnapshotClient*) arg;
	assert( self->m_fn_readable );
	return self->m_fn_readable(reader);
}

int SnapshotClient::timerAdapter(zloop_t* loop,int timer_id,void* arg) {
	(void)loop; (void) timer_id;
	SnapshotClient* self = (SnapshotClient*) arg;
	return self->onTimeoutTimer();
}

int SnapshotClient::onTimeoutTimer() {
	if( ri_time_now() > m_tv_timeout ) {
		LOG(ERROR) << "SnapshotClient timeout";
		auto ob = m_observer;
		stop();
		ob(SNAPSHOT_CLIENT_ERROR);
		return -1;
	}
	return 0;
}

int SnapshotClient::onReqReadable(zsock_t* sock) {
	int err = SNAPSHOT_CLIENT_ERROR;
	do {
		snapshot::SnapshotRep rep;
		if( -1 == zpb_recv(rep,sock) ) {
			LOG(ERROR) << "SnapshotClient recv SnapshotRep failed";
			break;
		}
		if( rep.result() != 0 || ! rep.has_address() ) {
			LOG(ERROR) << "SnapshotClient SnapshotRep error,result:" << rep.result() << ",address: " << (rep.has_address() ? rep.address() : "none");
			break;
		}
		zloop_reader_end(m_loop,m_sock);
		zsock_destroy(&m_sock);

		m_sock = zsock_new(ZMQ_PULL);
		assert(m_sock);

		if( -1 == zsock_connect(m_sock,"%s",rep.address().c_str()) ) {
			LOG(ERROR) << "SnapshotClient can NOT connect to: " << rep.address().c_str();
			break;
		}

		m_fn_readable = std::bind<int>(&SnapshotClient::pullSnapshotBegin,this,std::placeholders::_1);
		if( -1 == zloop_reader(m_loop,m_sock,readableAdapter,this) ) {
			break;
		}

		m_tv_timeout = ri_time_now() + 2000;
		return 0;
	} while( 0 );

	auto ob = m_observer;
	stop();
	ob(err);
	return -1;
}

bool SnapshotClient::isActive() const {
	return m_sock != nullptr;
}

