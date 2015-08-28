#include "ris/snapshot/snapshotserviceworker.h"
#include "zmqx/zprotobuf++.h"

SnapshotServiceWorker::SnapshotServiceWorker(const std::string& address) :
	m_address(address),
	m_actor(nullptr)
{
}

SnapshotServiceWorker::~SnapshotServiceWorker() {
	stop();
}


int SnapshotServiceWorker::start(const snapshot_package_t& snapshot) {
	if( m_actor != nullptr )
		return -1;

	if( snapshot.empty() )
		return -1;

	m_snapshot = snapshot;
	m_actor = zactor_new(actorAdapterFn,this);
	if( nullptr == m_actor ) {
		return -1;
	}
	if( 0 != zsock_wait(m_actor) ) {
		zactor_destroy(&m_actor);
		return -1;
	} else {
		return 0;
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
	zsock_signal(pipe,0);
	if( nullptr == sock ) {
		zsock_signal(pipe,1);
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
		if( -1 == zsock_bind(sock,"%s",m_address.c_str()) ) {
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
	if( m_snapshot.empty() ) {
		zsock_flush(sock);
		zstr_send(m_actor,"ok");
		return -1;
	} else {
		auto p = m_snapshot.front();
		m_snapshot.pop_front();
		return zpb_send(sock,*p);
	}
}


