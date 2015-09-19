#include <glog/logging.h>
#include "snapshot/snapshotclient.h"
#include "snapshot/snapshotclientworker.h"


SnapshotClient::SnapshotClient(zloop_t* loop) :
	m_caller_loop(loop),
	m_actor(nullptr)
{
}

SnapshotClient::~SnapshotClient() {
	stop();
}

int SnapshotClient::start(const std::function<void(int)>& completed,const std::shared_ptr<ISnapshotBuilder>& builder,const std::string& address) {
	if( m_actor )
		return -1;

	m_completed = completed;
	m_builder = builder;
	m_address = address;
	
	m_actor = zactor_new(&SnapshotClient::actorRunner,this);
	CHECK_NOTNULL( m_actor );

	m_actor_reader = std::make_shared<ZLoopReader>(m_caller_loop);
	m_actor_reader->start(zactor_sock(m_actor),std::bind<int>(&SnapshotClient::onCallerPipeReadable,this,std::placeholders::_1));
	return 0;
}


void SnapshotClient::stop() {
	if( m_actor_reader ) {
		m_actor_reader.reset();
	}
	if( m_actor ) {
		zactor_destroy(&m_actor);
	}
	m_completed = nullptr;
	m_builder.reset();
	m_address.clear();
}

int SnapshotClient::onCallerPipeReadable(zsock_t* sock) {
	DLOG(INFO) << "onCallerPipeReadable";
	int status = zsock_wait(sock);
	DLOG(INFO) << "onCallerPipeReadable wait status: " << status;
	auto completed = m_completed;
	stop();

	DLOG(INFO) << "onCallerPipeReadable call callback";
	completed(status);
	return 0;
}

int SnapshotClient::onRunnerPipeReadable(bool* running,zsock_t* sock) {
	DLOG(INFO) << "onRunnerPipeReadable";
	CHECK_NOTNULL(running);
	zmsg_t* msg = zmsg_recv(sock);
	zmsg_destroy(&msg);
	*running = false;
	return -1;
}

void SnapshotClient::onWorkerCompleted(zsock_t* pipe,int err) {
	DLOG(INFO) << "onWorkerCompleted: " << err;
	zsock_signal(pipe,err);
}

void SnapshotClient::run(zsock_t* pipe) {
	zsock_signal(pipe,0);
	zloop_t* loop = zloop_new();
	CHECK_NOTNULL(loop);

	do {
		bool running = true;
		auto reader = std::make_shared<ZLoopReader>(loop);
		auto worker = std::make_shared<SnapshotClientWorker>(loop);

		if( -1 == reader->start(pipe,std::bind<int>(&SnapshotClient::onRunnerPipeReadable,this,&running,std::placeholders::_1)) )
			break;

		if( -1 == worker->start(std::bind(&SnapshotClient::onWorkerCompleted,this,pipe,std::placeholders::_1),m_builder,m_address) )
			break;

		while( running ) {
			if( 0 == zloop_start(loop) ) {
				break;
			}
		}
		zloop_destroy(&loop);
		return;
	} while(0);

	zsock_signal(pipe,0);
	zloop_destroy(&loop);

	LOG(INFO) << "SnapshotClient shutdown";
}

void SnapshotClient::actorRunner(zsock_t* pipe,void* arg) {
	SnapshotClient* self = (SnapshotClient*)arg;
	self->run(pipe);
}

