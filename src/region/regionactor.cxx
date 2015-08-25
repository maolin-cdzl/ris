#include "ris/region/regionactor.h"


RIRegionActor::RIRegionActor() :
	m_actor(nullptr)
{
}

RIRegionActor::~RIRegionActor() {
	stop();
}

int RIRegionActor::start(const std::string& conf) {
	if( m_actor )
		return -1;
	if( -1 == loadConfig(conf) )
		return -1;

	m_actor = zactor_new(actorRunner,this);
	if( nullptr == m_actor )
		return -1;
	return 0;
}

int RIRegionActor::stop() {
	if( m_actor ) {
		zactor_destroy(&m_actor);
		return 0;
	} else {
		return -1;
	}
}

int RIRegionActor::loadConfig(const std::string& conf) {
}


void RIRegionActor::actorRunner(zsock_t* pipe,void* args) {
	int result = -1;
	RIRegionActor* self = (RIRegionActor*)args;
	zloop_t* loop = zloop_new();
	
	do {
		self->m_table = std::shared_ptr<RIRegionTable>( new RIRegionTable(self->m_region) );
		self->m_pub = std::shared_ptr<RIPublisher>( new RIPublisher(loop) );
		self->m_ssvc = std::shared_ptr<SnapshotService>( new SnapshotService(loop) );

		result = -1;
		do {
			if( -1 == self->m_pub->start(self->m_table,self->m_brokers) )
				break;
			if( -1 == self->m_ssvc->start(self->m_table,self->m_region.snapshot_address,self->m_snapshot_worker_address) )
				break;

			if( -1 == zloop_reader(loop,pipe,onPipeReadable,self) )
				break;
			result = 0;
		} while( 0 );

		if( -1 == result ) {
			zsock_signal(pipe,-1);
			break;
		}

		zsock_signal(pipe,0);
		

		while( self->m_running ) {
			result = zloop_start(loop);
			if( result == 0 ) {
				self->m_running = false;
				break;
			}
		}

	} while(0);

	zloop_reader_end(loop,pipe);
	self->m_pub.reset();
	self->m_ssvc.reset();
	self->m_table.reset();
	if( loop ) {
		zloop_destroy(&loop);
	}
}

int RIRegionActor::onPipeReadable(zloop_t* loop,zsock_t* reader,void* arg) {
}

