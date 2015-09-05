#include <glog/logging.h>
#include "snapshot/snapshotserviceworker.h"
#include "zmqx/zprotobuf++.h"

SnapshotServiceWorker::SnapshotServiceWorker(zloop_t* loop) :
	m_actor(nullptr),
	m_reader(loop)
{
}

SnapshotServiceWorker::~SnapshotServiceWorker() {
	stop();
}


int SnapshotServiceWorker::start(const std::string& address,const snapshot_package_t& snapshot,const std::function<void(int)>& cb) {
	if( m_actor != nullptr || snapshot.empty() )
		return -1;

	m_address = address;
	m_snapshot = snapshot;
	m_cb = cb;
	m_actor = zactor_new(actorAdapterFn,this);
	if( nullptr == m_actor ) {
		return -1;
	}
	m_reader.start(zactor_sock(m_actor),std::bind<int>(&SnapshotServiceWorker::onActorReadable,this,std::placeholders::_1));
	return 0;
}

int SnapshotServiceWorker::stop() {
	if( m_actor == nullptr )
		return -1;
	m_reader.stop();
	zactor_destroy(&m_actor);
	m_address.clear();
	m_endpoint.clear();
	m_cb = nullptr;
	return 0;
}

int SnapshotServiceWorker::onActorReadable(zsock_t* sock) {
	int err = -1;
	zmsg_t* msg = zmsg_recv(sock);
	if( msg ) {
		char* result = zframe_strdup( zmsg_first(msg) );
		LOG(INFO) << "SnapshotServiceWorker done with result: " << result;
		zmsg_destroy(&msg);
		if( strcmp(result,"ok") == 0 ) {
			err = 0;
		}
		free(result);
	}
	auto cb = m_cb;
	stop();
	cb(err);
	return 0;
}

void SnapshotServiceWorker::actorAdapterFn(zsock_t* pipe,void* arg) {
	((SnapshotServiceWorker*)arg)->run(pipe);
}

void SnapshotServiceWorker::run(zsock_t* pipe) {
	zsock_t* sock = createPipelineSock();
	assert( sock );

	zmq_pollitem_t pollitems[2];
	pollitems[0].socket = zsock_resolve(pipe);
	pollitems[0].fd = 0;
	pollitems[0].events = ZMQ_POLLIN;

	pollitems[1].socket = zsock_resolve(sock);
	pollitems[1].fd = 0;
	pollitems[1].events = ZMQ_POLLOUT;

	zsock_signal(pipe,0);
	int result = 0;
	do {
		result = zmq_poll(pollitems,2,5000);
		if( result == 0 ) {
			// error or timeout,log it
			LOG(ERROR) << "SnapshotServiceWorker poll timeout";
			zstr_send(pipe,"timeout");
			break;
		} else if( result == -1 ) {
			int e = errno;
			LOG(ERROR) << "SnapshotServiceWorker poll error: " << e;
			zstr_sendf(pipe,"error: %i",e);
			break;
		}

		if( pollitems[0].revents & ZMQ_POLLIN ) {
			if( -1 == onPipeReadable(sock,pipe) )
				break;
		} else if( pollitems[1].revents & ZMQ_POLLOUT ) {
			if( -1 == onPipelineWritable(sock,pipe) )
				break;
		}
	} while(true);

	zsock_destroy(&sock);
	LOG(INFO) << "SnapshotServiceWorker exit";
}


zsock_t* SnapshotServiceWorker::createPipelineSock() {
	zsock_t* sock = nullptr;
	
	do {
		sock = zsock_new(ZMQ_PUSH);
		if( -1 == zsock_bind(sock,"%s",m_address.c_str()) ) {
			LOG(FATAL) << "SnapshotServiceWorker can NOT bind to: " << m_address;
			break;
		}
		// set linger so when we close it,it will not discard pending message.
		zsock_set_linger(sock,20);
		m_endpoint = zsock_endpoint(sock);
		if( m_endpoint.empty() )
			break;
		
		LOG(INFO) << "SnapshotServiceWorker bind to: " << m_endpoint;
		return sock;
	} while(0);

	if( sock ) {
		zsock_destroy(&sock);
	}
	return nullptr;
}

int SnapshotServiceWorker::onPipeReadable(zsock_t* sock,zsock_t* pipe) {
	(void)sock;
	zmsg_t* msg = zmsg_recv(pipe);
	assert( msg );
	if( zframe_streq( zmsg_first(msg), "$TERM" ) ) {
		LOG(INFO) << "SnapshotServiceWorker pipe recv TERM message";
	} else {
		// log it
		LOG(INFO) << "SnapshotServiceWorker pipe recv unknown message";
	}
	zmsg_destroy(&msg);
	return -1;
}

int SnapshotServiceWorker::onPipelineWritable(zsock_t* sock,zsock_t* pipe) {
	if( m_snapshot.empty() ) {
		LOG(INFO) << "SnapshotServiceWorker send all snapshot item done";
		zstr_send(pipe,"ok");
		return -1;
	} else {
		auto p = m_snapshot.front();
		m_snapshot.pop_front();
		if( -1 == zpb_send(sock,*p) ) {
			int err = errno;
			LOG(ERROR) << "SnapshotServiceWorker send snapshot item error: " << err;
			return -1;
		} else {
			DLOG(INFO) << "SnapshotServiceWorker send snapshot item";
			return 0;
		}
	}
}


