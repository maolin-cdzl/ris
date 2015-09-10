#include <iostream>
#include <glog/logging.h>
#include "snapshot/snapshotclient.h"
#include "ris/snapshot.pb.h"
#include "zmqx/zhelper.h"
#include "zmqx/zprotobuf++.h"


SnapshotClient::SnapshotClient(zloop_t* loop) :
	m_loop(loop),
	m_timer(loop),
	m_reader(loop)
{
}

SnapshotClient::~SnapshotClient() {
	stop();
}

int SnapshotClient::start(const std::function<void(int)>& ob,const std::shared_ptr<ISnapshotBuilder>& builder,const std::string& address) {
	if( m_reader.isActive() )
		return -1;
	
	zsock_t* req = nullptr;
	do {
		req = zsock_new_req(address.c_str());
		if( nullptr == req ) {
			LOG(ERROR) << "SnapshotClient can NOT connect to: " << address;
			break;
		}

		snapshot::SnapshotReq msg;
		if( -1 == zpb_send(req,msg) ) {
			LOG(ERROR) << "SnapshotClient send SnapshotReq failed";
			break;
		}

		if( -1 == m_reader.start(&req,std::bind<int>(&SnapshotClient::onReqReadable,this,std::placeholders::_1),"Request") ) {
			LOG(FATAL) << "SnapshotClient start loop reader failed: " << errno;
			break;
		}
		
		if( -1 == m_timer.start(500,3000,std::bind<int>(&SnapshotClient::onTimeoutTimer,this)) ) {
			LOG(FATAL) << "SnapshotClient register loop timer failed: " << errno;
			break;
		}
		LOG(INFO) << "SnapshotClient send request to address: " << address;
		m_builder = builder;
		m_observer = ob;
		m_last_region.clear();
		return 0;
	} while( 0 );

	if( req ) {
		zsock_destroy(&req);
	}

	stop();
	return -1;
}

void SnapshotClient::stop() {
	m_reader.stop();
	m_timer.stop();
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

		m_reader.rebind(std::bind<int>(&SnapshotClient::pullRegionBegin,this,std::placeholders::_1),"pullRegionBegin");
		m_timer.delay(3000);
		return 0;
	} while(0);

	auto ob = m_observer;
	stop();
	ob(SNAPSHOT_CLIENT_ERROR);
	return -1;
}

int SnapshotClient::pullRegionBegin(zsock_t* sock) {
	do {
		snapshot::RegionBegin msg;
		if( -1 == zpb_recv(msg,sock) ) {
			LOG(ERROR) << "SnapshotClient recv RegionBegin failed";
			break;
		}

		Region region;
		region.id = msg.uuid();
		region.version = msg.version();
		region.idc = msg.idc();
		region.bus_address = msg.bus_address();
		region.snapshot_address = msg.snapshot_address();

		const ri_time_t now = ri_time_now();
		region.timeval = now;

		DLOG(INFO) << "SnapshotClient recv RegionBegin: " << region.id << "(" << region.version << ")"; 
		if( -1 == m_builder->addRegion(region) ) {
			LOG(ERROR) << "SnapshotClient addRegion failed,region: " <<  region.id << "(" << region.version << ")";
			break;
		}
		m_last_region = msg.uuid();
		m_reader.rebind(std::bind<int>(&SnapshotClient::pullRegionBody,this,std::placeholders::_1),"pullRegionBody");
		m_timer.delay(3000);
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
			m_reader.rebind(std::bind<int>(&SnapshotClient::pullRegionBody,this,std::placeholders::_1),"pullRegionBody");
			m_timer.delay(3000);
		} else if( msg->GetDescriptor() == snapshot::SnapshotEnd::descriptor() ) {
			DLOG(INFO) << "SnapshotClient recv SnapshotEnd"; 
			m_reader.stop();
			m_timer.stop();
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
	int result = -1;
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
			m_timer.delay(3000);
			result = 0;
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
			m_timer.delay(3000);
			result = 0;
		} else if( msg->GetDescriptor() == snapshot::RegionEnd::descriptor() ) {
			DLOG(INFO) << "SnapshotClient recv RegionEnd: ";
			auto p = std::dynamic_pointer_cast<snapshot::RegionEnd>(msg);
			if( p->uuid() != m_last_region )
				break;
			m_last_region.clear();
			m_reader.rebind(std::bind<int>(&SnapshotClient::pullRegionOrFinish,this,std::placeholders::_1),"pullRegionOrFinish");
			m_timer.delay(3000);
			result = 0;
		} else {
			LOG(ERROR) << "SnapshotClient pullRegionBody recv unexpect message: " << msg->GetTypeName();
			break;
		}
	} while(0);

	if( -1 == result ) {
		cancelRegion();
		auto ob = m_observer;
		stop();
		ob(SNAPSHOT_CLIENT_ERROR);
	}
	return result;
}

int SnapshotClient::onTimeoutTimer() {
	LOG(ERROR) << "SnapshotClient timeout,state: " << state();
	cancelRegion();
	auto ob = m_observer;
	stop();
	ob(SNAPSHOT_CLIENT_ERROR);
	return -1;
}

int SnapshotClient::onReqReadable(zsock_t* sock) {
	int err = SNAPSHOT_CLIENT_ERROR;
	zsock_t* pull = nullptr;
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
		LOG(INFO) << "SnapshotClient get response to address: " << rep.address();
		m_reader.stop();

		pull = zsock_new(ZMQ_PULL);
		assert(pull);
		if( -1 == zsock_connect(pull,"%s",rep.address().c_str()) ) {
			LOG(ERROR) << "SnapshotClient can NOT connect to: " << rep.address().c_str();
			break;
		}

		m_reader.start(&pull,std::bind<int>(&SnapshotClient::pullSnapshotBegin,this,std::placeholders::_1),"pullSnapshotBegin");

		m_timer.delay(3000);
		return 0;
	} while( 0 );

	if( pull ) {
		zsock_destroy(&pull);
	}

	auto ob = m_observer;
	stop();
	ob(err);
	return -1;
}

bool SnapshotClient::isActive() const {
	return m_reader.isActive();
}

std::string SnapshotClient::state() const {
	return m_reader.state();
}

void SnapshotClient::cancelRegion() {
	if( ! m_last_region.empty() ) {
		m_builder->rmRegion(m_last_region);
		m_last_region.clear();
	}
}

