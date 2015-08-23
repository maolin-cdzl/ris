#include "ris/ritypes.h"
#include "ris/snapshot/snapshotserviceworker.h"

SnapshotServiceWorker::SnapshotServiceWorker(const std::string& express) :
	m_express(express),
	m_actor(nullptr)
{
}

SnapshotServiceWorker::~SnapshotServiceWorker() {
	stop();
}


int SnapshotServiceWorker::start(const std::shared_ptr<Snapshot>& snapshot) {
	if( m_actor != nullptr )
		return -1;

	if( transSnapshot(snapshot) == -1 )
		return -1;

	m_actor = zactor_new(actorAdapterFn,this);
	if( m_actor ) {
		return 0;
	} else {
		return -1;
	}
}

int SnapshotServiceWorker::stop() {
	if( m_actor == nullptr )
		return -1;

	zactor_destroy(&m_actor);
	return 0;
}

void SnapshotServiceWorker::actorAdapterFn(zsock_t* pipe,void* arg) {
	((SnapshotServiceWorker*)arg)->run(pipe);
}

void SnapshotServiceWorker::run(zsock_t* pipe) {
	zsock_t* sock = createPipelineSock();
	if( nullptr == sock ) {
		zsock_signal(pipe,-1);
		return;
	} else {
		zsock_signal(pipe,0);
	}

	zmq_pollitem_t pollitems[2];
	pollitems[0].socket = zsock_resolve(pipe);
	pollitems[0].fd = 0;
	pollitems[0].events = ZMQ_POLLIN;

	pollitems[1].socket = zsock_resolve(sock);
	pollitems[1].fd = 0;
	pollitems[1].events = ZMQ_POLLOUT;

	int result = 0;
	do {
		result = zmq_poll(pollitems,2,5000);
		if( result == 0 ) {
			// error or timeout,log it
			zstr_send(m_actor,"timeout");
			break;
		} else if( result == -1 ) {
			zstr_sendf(m_actor,"error: %i",errno);
			break;
		}

		if( pollitems[0].revents & ZMQ_POLLIN ) {
			if( -1 == onPipeReadable(pipe) )
				break;
		} else if( pollitems[1].revents & ZMQ_POLLOUT ) {
			if( -1 == onPipelineWritable(sock) )
				break;
		}
	} while(0);

	zsock_destroy(&sock);
}


zsock_t* SnapshotServiceWorker::createPipelineSock() {
	zsock_t* sock = nullptr;
	
	do {
		sock = zsock_new(ZMQ_PUSH);
		if( -1 == zsock_bind(sock,"%s",m_express.c_str()) ) {
			break;
		}
		m_endpoint = zsock_endpoint(sock);
		if( m_endpoint.empty() )
			break;

		return sock;
	} while(0);

	if( sock ) {
		zsock_destroy(&sock);
	}
	return nullptr;
}

int SnapshotServiceWorker::onPipeReadable(zsock_t* pipe) {
	zmsg_t* msg = zmsg_recv(pipe);
	assert( msg );
	if( zframe_streq( zmsg_first(msg), "$TERM" ) ) {
		// log it
	} else {
		// log it
	}
	zmsg_destroy(&msg);
	return -1;
}

int SnapshotServiceWorker::onPipelineWritable(zsock_t* sock) {
	if( m_msgs.empty() ) {
		zsock_flush(sock);
		zstr_send(m_actor,"ok");
		return -1;
	} else {
		zmsg_t* msg = m_msgs.front();
		m_msgs.pop_front();
		return zmsg_send(&msg,sock);
	}
}

int SnapshotServiceWorker::transSnapshotPartition(std::shared_ptr<SnapshotPartition>& part) {
	zmsg_t* msg = nullptr;

	//  part header
	msg = zmsg_new();
	if( part->package(msg) == -1 ) {
		zmsg_destroy(&msg);
		return -1;
	}
	m_msgs.push_back(msg);

	bool good = false;
	do {
		auto it = part->popItem();
		if( it == nullptr ) {
			good = true;
			break;
		}

		msg = zmsg_new();
		if( it->package(msg) == -1 ) {
			zmsg_destroy(&msg);
			break;
		}
		m_msgs.push_back(msg);
	} while(true);

	if( good ) {
		//  part border 
		msg = zmsg_new();
		if( part->packageBorder(msg) == -1 ) {
			zmsg_destroy(&msg);
			return -1;
		}
		m_msgs.push_back(msg);
	}
	return -1;
}

int SnapshotServiceWorker::transSnapshot(std::shared_ptr<Snapshot>& snapshot) {
	if( snapshot == nullptr || ! m_msgs.empty() ) {
		return -1;
	}
	zmsg_t* msg = nullptr;
	
	do {
		// snapshot header
		msg = zmsg_new();
		if( snapshot->package(msg) == -1 )
			break;
		m_msgs.push_back(msg);
		msg = nullptr;

		bool good = false;
		while(1) {
			auto part = snapshot->popPartition();
			if( part == nullptr ) {
				good = true;
				break;
			}
			
			if( -1 == transSnapshotPartition(part) )
				break;
		}

		if( ! good )
			break;

		// snapshot border
		msg = zmsg_new();
		if( snapshot->packageBorder(msg) == -1 )
			break;
		m_msgs.push_back(msg);
		msg = nullptr;

		return 0;
	} while(0);

	if( msg ) {
		zmsg_destroy(&msg);
	}

	while( ! m_msgs.empty() ) {
		auto msg = m_msgs.front();
		m_msgs.pop_front();
		zmsg_destroy(&msg);
	}
	return -1;
}


