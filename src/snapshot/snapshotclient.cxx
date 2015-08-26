#include "ris/snapshot/snapshotclient.h"
#include "ris/zmqhelper.h"
#include "ris/snapshot/snapshot.h"


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

		if( -1 == pullSnapshot(sock) ) {
			break;
		}
		
		assert(m_snapshot);
		result = builder->build(m_snapshot);
		m_snapshot.reset();
	} while( 0 );

	if( msg ) {
		zmsg_destroy(&msg);
	}
	if( sock ) {
		zsock_destroy(&sock);
	}

	return result;
}

int SnapshotClient::pullSnapshotHeader(zsock_t* sock) {
	zmsg_t* msg = nullptr;
	assert( m_snapshot == nullptr );
	do {
		if( zmq_wait_readable(sock,5000) <= 0 ) {
			break;
		}
		msg = zmsg_recv(sock);
		if( msg == nullptr ) {
			break;
		}
		if( ! Snapshot::is(msg) ) {
			break;
		}

		m_snapshot = Snapshot::parse(msg);
		if( ! m_snapshot ) {
			break;
		}
		zmsg_destroy(&msg);
		return 0;
	} while(0);

	if( msg ) {
		zmsg_destroy(&msg);
	}
	return -1;
}

int SnapshotClient::pullPartitionOrFinish(zsock_t* sock) {
	zmsg_t* msg = nullptr;
	int result = -1;
	assert( m_part == nullptr );

	do {
		if( zmq_wait_readable(sock,5000) <= 0 ) {
			break;
		}
		msg = zmsg_recv(sock);
		if( msg == nullptr ) {
			break;
		}
		if( SnapshotPartition::is(msg) ) {
			m_part = SnapshotPartition::parse(msg);
			if( m_part ) {
				result = 0;
			}
		} else if( Snapshot::isBorder(msg) ) {
			result = 1;
		}

		zmsg_destroy(&msg);
	} while(0);

	if( msg ) {
		zmsg_destroy(&msg);
	}
	return result;
}

int SnapshotClient::pullPartitionBody(zsock_t* sock) {
	zmsg_t* msg = nullptr;
	int result = -1;
	assert( m_part );

	do {
		if( zmq_wait_readable(sock,5000) <= 0 ) {
			break;
		}
		msg = zmsg_recv(sock);
		if( msg == nullptr ) {
			break;
		}
		if( SnapshotItem::is(msg) ) {
			auto it = SnapshotItem::parse(msg);
			if( it == nullptr )
				break;
			m_part->addItem(it);
		} else if( SnapshotPartition::isBorder(msg) ) {
			m_snapshot->addPartition(m_part);
			m_part.reset();
			result = 0;
			break;
		} else {
			break;
		}

		zmsg_destroy(&msg);
		return 0;
	} while(1);

	if( msg ) {
		zmsg_destroy(&msg);
	}
	return result;
}

int SnapshotClient::pullSnapshot(zsock_t* sock) {
	int result = -1;
	assert( m_snapshot == nullptr );

	if( -1 == pullSnapshotHeader(sock) ) {
		return -1;
	}

	do {
		result = pullPartitionOrFinish(sock);
		if( 1 == result ) {
			result = 0;
			break;
		} else if( 0 == result ) {
			if( -1 == pullPartitionBody(sock) ) {
				break;
			}
		} else {
			break;
		}
	} while( 1 );

	return result;
}


