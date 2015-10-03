#include <iostream>
#include <glog/logging.h>
#include "snapshot/snapshotclientworker.h"
#include "ris/snapshot.pb.h"
#include "zmqx/zhelper.h"
#include "zmqx/zprotobuf++.h"


SnapshotClientWorker::SnapshotClientWorker(zloop_t* loop) :
	m_loop(loop),
	m_timer(loop),
	m_reader(loop)
{
}

SnapshotClientWorker::~SnapshotClientWorker() {
	stop();
}

int SnapshotClientWorker::start(const std::function<void(int)>& completed,const std::shared_ptr<ISnapshotBuilder>& builder,const std::string& address) {
	if( m_reader.isActive() )
		return -1;
	
	zsock_t* sock = nullptr;
	do {
		m_requester = new_short_identity();
		sock = zsock_new(ZMQ_DEALER);
		CHECK_NOTNULL(sock);
		zsock_set_identity(sock,m_requester.c_str());
		if( -1 == zsock_connect(sock,"%s",address.c_str()) ) {
			LOG(ERROR) << "SnapshotClientWorker can not connect to: " << address;
			break;
		}

		snapshot::SnapshotReq msg;
		msg.set_requester(m_requester);
		if( -1 == zpb_send(sock,msg,true) ) {
			LOG(ERROR) << "SnapshotClientWorker send SnapshotReq failed";
			break;
		}

		if( -1 == m_reader.start(&sock,std::bind<int>(&SnapshotClientWorker::onReqReadable,this,std::placeholders::_1),"Request") ) {
			LOG(FATAL) << "SnapshotClientWorker start loop reader failed: " << errno;
			break;
		}
		
		if( -1 == m_timer.start(500,3000,std::bind<int>(&SnapshotClientWorker::onTimeoutTimer,this)) ) {
			LOG(FATAL) << "SnapshotClientWorker register loop timer failed: " << errno;
			break;
		}
		LOG(INFO) << "SnapshotClientWorker send request to address: " << address;
		m_completed = completed;
		m_builder = builder;
		m_last_region.clear();
		return 0;
	} while( 0 );

	if( sock ) {
		zsock_destroy(&sock);
	}

	stop();
	return -1;
}

void SnapshotClientWorker::stop() {
	m_reader.stop();
	m_timer.stop();
	m_builder.reset();
	m_completed = nullptr;
	m_last_region.clear();
	m_requester.clear();
}

int SnapshotClientWorker::pullSnapshotBegin(zsock_t* sock) {
	do {
		snapshot::SnapshotBegin msg;
		if( -1 == zpb_recv(msg,sock) ) {
			LOG(ERROR) << "SnapshotClientWorker recv SnapshotBegin failed";
			break;
		}

		m_reader.rebind(std::bind<int>(&SnapshotClientWorker::pullRegionBegin,this,std::placeholders::_1),"pullRegionBegin");
		m_timer.delay(3000);
		return 0;
	} while(0);

	finish(-1);
	return -1;
}

int SnapshotClientWorker::pullRegionBegin(zsock_t* sock) {
	do {
		snapshot::RegionBegin msg;
		if( -1 == zpb_recv(msg,sock) ) {
			LOG(ERROR) << "SnapshotClientWorker recv RegionBegin failed";
			break;
		}

		Region region(msg.region());
		region.version = msg.rt().version();

		DLOG(INFO) << "SnapshotClientWorker recv RegionBegin: " << region.id << "(" << region.version << ")"; 
		if( -1 == m_builder->addRegion(region) ) {
			LOG(ERROR) << "SnapshotClientWorker addRegion failed,region: " <<  region.id << "(" << region.version << ")";
			break;
		}
		m_last_region = region.id;
		m_reader.rebind(std::bind<int>(&SnapshotClientWorker::pullRegionBody,this,std::placeholders::_1),"pullRegionBody");
		m_timer.delay(3000);
		return 0;
	} while(0);

	finish(-1);
	return -1;
}

int SnapshotClientWorker::pullRegionOrFinish(zsock_t* sock) {
	CHECK( m_last_region.empty() );

	do {
		auto msg = zpb_recv(sock);
		if( msg == nullptr ) {
			LOG(ERROR) << "SnapshotClientWorker recv RegionBegin or SnapshotEnd failed";
			break;
		}
		if( msg->GetDescriptor() == snapshot::RegionBegin::descriptor() ) {
			auto p = std::dynamic_pointer_cast<snapshot::RegionBegin>(msg);
			Region region(p->region());
			region.version = p->rt().version();

			if( ! region.good() ) {
				LOG(ERROR) << "RegionBegin is uncompleted";
				break;
			}

			const ri_time_t now = ri_time_now();
			region.timeval = now;

			DLOG(INFO) << "SnapshotClientWorker recv RegionBegin: " << region.id << "(" << region.version << ")"; 
			if( -1 == m_builder->addRegion(region) ) {
				LOG(ERROR) << "SnapshotClientWorker addRegion failed,region: " <<  region.id << "(" << region.version << ")";
				break;
			}
			m_last_region = region.id;
			m_reader.rebind(std::bind<int>(&SnapshotClientWorker::pullRegionBody,this,std::placeholders::_1),"pullRegionBody");
			m_timer.delay(3000);
		} else if( msg->GetDescriptor() == snapshot::SnapshotEnd::descriptor() ) {
			DLOG(INFO) << "SnapshotClientWorker recv SnapshotEnd"; 
			m_reader.stop();
			m_timer.stop();
			finish(0);
		} else if( msg->GetDescriptor() == snapshot::SyncSignalReq::descriptor() ) {
			DLOG(INFO) << "SnapshotClientWorker recv sync signal";
			snapshot::SyncSignalRep sync;
			sync.set_requester(m_requester);

			zpb_send(m_reader.socket(),sync,true);
			m_timer.delay(3000);
		} else {
			LOG(ERROR) << "SnapshotClientWorker recv unexpect message: " << msg->GetTypeName() << ", when wait RegionBegin or SnapshotEnd";
		}
		return 0;
	} while(0);

	finish(-1);
	return -1;
}

int SnapshotClientWorker::pullRegionBody(zsock_t* sock) {
	CHECK( ! m_last_region.empty() );
	do {
		auto msg = zpb_recv(sock);
		if( msg == nullptr ) {
			LOG(ERROR) << "SnapshotClientWorker recv Service or Payload failed";
			break;
		}
		if( msg->GetDescriptor() == ris::Payload::descriptor() ) {
			auto p = std::dynamic_pointer_cast<ris::Payload>(msg);
			Payload pl(*p);
			if( !pl.good() ) {
				LOG(ERROR) << "payload not good";
				break;
			}
			DLOG(INFO) << "SnapshotClientWorker recv Payload: " << pl.id; 
			if( -1 == m_builder->addPayload(m_last_region,pl) ) {
				LOG(ERROR) << "Snapshot addPayload failed: " << pl.id;
				break;
			}
			m_timer.delay(3000);
		} else if( msg->GetDescriptor() == ris::Service::descriptor() ) {
			auto p = std::dynamic_pointer_cast<ris::Service>(msg);
			Service svc(*p);
			if( !svc.good() ) {
				LOG(ERROR) << "service not good";
				break;
			}

			DLOG(INFO) << "SnapshotClientWorker recv Service: " << svc.name; 
			if( -1 == m_builder->addService(m_last_region,svc) ) {
				LOG(ERROR) << "Snapshot addService failed: " << svc.name;
				break;
			}
			m_timer.delay(3000);
		} else if( msg->GetDescriptor() == snapshot::RegionEnd::descriptor() ) {
			DLOG(INFO) << "SnapshotClientWorker recv RegionEnd: ";
			auto p = std::dynamic_pointer_cast<snapshot::RegionEnd>(msg);
			if( p->rt().uuid() != m_last_region ) {
				break;
			}
			m_last_region.clear();
			m_reader.rebind(std::bind<int>(&SnapshotClientWorker::pullRegionOrFinish,this,std::placeholders::_1),"pullRegionOrFinish");
			m_timer.delay(3000);
		} else if( msg->GetDescriptor() == snapshot::SyncSignalReq::descriptor() ) {
			DLOG(INFO) << "SnapshotClientWorker recv sync signal";
			snapshot::SyncSignalRep sync;
			sync.set_requester(m_requester);

			zpb_send(m_reader.socket(),sync,true);
			m_timer.delay(3000);
		} else {
			LOG(ERROR) << "SnapshotClientWorker pullRegionBody recv unexpect message: " << msg->GetTypeName();
			break;
		}
		return 0;
	} while(0);

	finish(-1);
	return -1;
}

int SnapshotClientWorker::onTimeoutTimer() {
	LOG(ERROR) << "SnapshotClientWorker timeout,state: " << state();
	finish(-1);
	return -1;
}

int SnapshotClientWorker::onReqReadable(zsock_t* sock) {
	do {
		snapshot::SnapshotRep rep;
		if( -1 == zpb_recv(rep,sock) ) {
			LOG(ERROR) << "SnapshotClientWorker recv SnapshotRep failed";
			break;
		}
		if( rep.result() != 0 ) {
			LOG(ERROR) << "SnapshotClientWorker SnapshotRep error,result:" << rep.result();
			break;
		}


		m_reader.rebind(std::bind<int>(&SnapshotClientWorker::pullSnapshotBegin,this,std::placeholders::_1),"pullSnapshotBegin");
		m_timer.delay(3000);
		return 0;
	} while( 0 );

	finish(-1);
	return -1;
}

bool SnapshotClientWorker::isActive() const {
	return m_reader.isActive();
}

std::string SnapshotClientWorker::state() const {
	return m_reader.state();
}

void SnapshotClientWorker::finish(int err) {
	auto completed = m_completed;
	stop();
	completed(err);
}

