#include "ris/snapshot/snapshotclient.h"
#include "ris/snapshot.pb.h"
#include "zmqx/zhelper.h"
#include "zmqx/zprotobuf++.h"


SnapshotClient::SnapshotClient()
{
}

SnapshotClient::~SnapshotClient() {
}

int SnapshotClient::requestSnapshot(const std::shared_ptr<ISnapshotBuilder>& builder,const std::string& address) {
	assert( builder );
	assert( !address.empty() );

	int result = -1;
	zsock_t* sock = nullptr;
	zmsg_t* msg = nullptr;
	m_builder = builder;

	do {
		sock = zsock_new_req(address.c_str());
		if( nullptr == sock ) {
			break;
		}

		if( zmq_wait_writable(sock,5000) <= 0 ) {
			break;
		}

		if( -1 == zstr_send(sock,"$ssreq") ) {
			break;
		}
	
		if( zmq_wait_readable(sock,5000) <= 0 ) {
			break;
		}
		msg = zmsg_recv(sock);
		if( msg == nullptr )
			break;
		if( ! zframe_streq( zmsg_first(msg),"ok" ) ) {
			break;
		}
		char* workeraddr = zframe_strdup(zmsg_next(msg));
		zmsg_destroy(&msg);
		zsock_destroy(&sock);

		sock = zsock_new_pull(workeraddr);
		free(workeraddr);
		if( nullptr == sock ) {
			break;
		}

		result = pullSnapshot(sock);
	} while( 0 );

	if( msg ) {
		zmsg_destroy(&msg);
	}
	if( sock ) {
		zsock_destroy(&sock);
	}
	m_builder.reset();
	return result;
}

int SnapshotClient::pullSnapshotBegin(zsock_t* sock) {
	do {
		if( zmq_wait_readable(sock,5000) <= 0 ) {
			break;
		}
		snapshot::SnapshotBegin msg;
		if( -1 == zpb_recv(msg,sock) ) {
			break;
		}

		return 0;
	} while(0);

	return -1;
}

int SnapshotClient::pullRegionOrFinish(zsock_t* sock) {
	int result = -1;
	assert( m_last_region.empty() );

	do {
		if( zmq_wait_readable(sock,5000) <= 0 ) {
			break;
		}

		auto msg = zpb_recv(sock);
		if( msg == nullptr ) {
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
			if( p->has_msg_address() ) {
				region.msg_address = p->msg_address();
			}
			if( p->has_snapshot_address() ) {
				region.snapshot_address = p->snapshot_address();
			}
			region.timeval = ri_time_now();

			m_last_region = p->uuid();
			result = m_builder->addRegion(region);
		} else if( msg->GetDescriptor() == snapshot::SnapshotEnd::descriptor() ) {
			result = 1;
		}

	} while(0);

	return result;
}

int SnapshotClient::pullRegionBody(zsock_t* sock) {
	int result = -1;
	assert( ! m_last_region.empty() );

	do {
		if( zmq_wait_readable(sock,5000) <= 0 ) {
			break;
		}

		auto msg = zpb_recv(sock);
		if( msg == nullptr ) {
			break;
		}
		if( msg->GetDescriptor() == snapshot::Payload::descriptor() ) {
			auto p = std::dynamic_pointer_cast<snapshot::Payload>(msg);
			Payload pl;
			pl.id = p->uuid();
			pl.timeval = ri_time_now();
			if( -1 == m_builder->addPayload(m_last_region,pl) )
				break;
		} else if( msg->GetDescriptor() == snapshot::Service::descriptor() ) {
			auto p = std::dynamic_pointer_cast<snapshot::Service>(msg);
			Service svc;
			svc.name = p->name();
			svc.address = p->address();
			svc.timeval = ri_time_now();
			if( -1 == m_builder->addService(m_last_region,svc) ) {
				break;
			}
		} else if( msg->GetDescriptor() == snapshot::RegionEnd::descriptor() ) {
			m_last_region.clear();
			result = 0;
			break;
		} else {
			break;
		}
	} while(1);

	return result;
}

int SnapshotClient::pullSnapshot(zsock_t* sock) {
	int result = -1;

	if( -1 == pullSnapshotBegin(sock) ) {
		return -1;
	}

	do {
		result = pullRegionOrFinish(sock);
		if( 1 == result ) {
			result = 0;
			break;
		} else if( 0 == result ) {
			if( -1 == pullRegionBody(sock) ) {
				break;
			}
		} else {
			break;
		}
	} while( 1 );

	return result;
}


