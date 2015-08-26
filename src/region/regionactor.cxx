#include "ris/region/regionactor.h"
#include <glog/logging.h>
#include <libconfig.h++>


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
	libconfig::Config cfg;

	std::string regid,idc,msgaddr,ssaddr,workeraddr;
	std::list<std::string> brokers;

	try {
		cfg.readFile(conf.c_str());
	} catch( const libconfig::FileIOException& e ) {
		LOG(FATAL) << "RIRegionActor can not open config file: " << conf;
		return -1;
	} catch( const libconfig::ParseException& e ) {
		LOG(FATAL) << "Parse error at " << e.getFile() << ":" << e.getLine() << " - " << e.getError();
		return -1;
	}

	try {
		const libconfig::Setting& region = cfg.lookup("region");
		regid = region["id"].c_str();
		idc = region["idc"].c_str();
		msgaddr = region["msgaddress"].c_str();
	} catch( const libconfig::SettingNotFoundException& e ) {
		LOG(FATAL) << "Can not found Region setting: " << e.what();
		return -1;
	} catch( const libconfig::SettingTypeException& e ) {
		LOG(FATAL) << "Error when parse Region: " << e.what();
		return -1;
	}

	try {
		if( cfg.exists("region.snapshot") ) {
			const libconfig::Setting& snapshot = cfg.lookup("region.snapshot");
			ssaddr = snapshot["address"].c_str();
			workeraddr = snapshot["workeraddress"].c_str();
		}
	} catch( const libconfig::SettingNotFoundException& e ) {
		LOG(FATAL) << "Can not found Snapshot setting: " << e.what();
		return -1;
	} catch( const libconfig::SettingTypeException& e ) {
		LOG(FATAL) << "Error when parse Snapshot: " << e.what();
		return -1;
	}

	try {
		const libconfig::Setting& broker = cfg.lookup("ribrokers");
		const int count = broker.getLength();
		for(int i=0; i < count; ++i) {
			std::string b = broker[i];
			brokers.push_back(b);
		}
	} catch( const libconfig::SettingNotFoundException& e ) {
		LOG(FATAL) << "Can not found brokers setting: " << e.what();
		return -1;
	} catch( const libconfig::SettingTypeException& e ) {
		LOG(FATAL) << "Error when parse Snapshot: " << e.what();
		return -1;
	}

	m_region.id = regid;
	m_region.idc = idc;
	m_region.msg_address = msgaddr;
	m_brokers = brokers;
	if( ! ssaddr.empty() ) {
		m_region.snapshot_address = ssaddr;
		m_snapshot_worker_address = workeraddr;
	}

	return 0;
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

			if( -1 == zloop_reader(loop,pipe,pipeReadableAdapter,self) )
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

int RIRegionActor::pipeReadableAdapter(zloop_t* loop,zsock_t* reader,void* arg) {
	RIRegionActor* self = (RIRegionActor*)arg;
	return self->onPipeReadable(reader);
}

int RIRegionActor::onPipeReadable(zsock_t* pipe) {
	zmsg_t* msg = zmsg_recv(pipe);
	char* str = nullptr;

	do {
		if( nullptr == msg ) {
			LOG(WARNING) << "RIRegionActor recv empty message";
			break;
		}

		zframe_t* fr = zmsg_first(msg);
		if( zmsg_size(msg) == 1 ) {
			if( zframe_streq( fr, "$TERM") ) {
				LOG(INFO) << m_region.id << " terminated"; 
				m_running = false;
			} else {
				str = zframe_strdup(fr);
				LOG(WARNING) << "RIRegionActor recv unknown message: " << str;
			}
			break;
		}

		if( zframe_streq(fr,"#pld") ) {
			fr = zmsg_next(msg);
			str = zframe_strdup(fr);
			Payload pl;
			pl.id = str;
			m_table->newPayload(pl);
		} else if( zframe_streq(fr,"#delpld") ) {
			fr = zmsg_next(msg);
			str = zframe_strdup(fr);
			m_table->delPayload(str);
		} else if( zframe_streq(fr,"#svc") ) {
			Service svc;
			fr = zmsg_next(msg);
			str = zframe_strdup(fr);
			svc.id = str;
			free(str);
			str = nullptr;

			fr = zmsg_next(msg);
			if( fr ) {
				str = zframe_strdup(fr);
				svc.address = str;
				free(str);
				str = nullptr;
				m_table->newService(svc);
			} else {
				LOG(WARNING) << "RIRegionActor recv bad message for new service";
			}

		} else if( zframe_streq(fr,"#delsvc") ) {
			fr = zmsg_next(msg);
			str = zframe_strdup(fr);
			m_table->delService(str);
		} else {
			str = zframe_strdup(fr);
			LOG(WARNING) << "RIRegionActor recv unknown message: " << str;
		}
	} while( 0 );

	if( str ) {
		free(str);
	}
	if( msg ) {
		zmsg_destroy(&msg);
	}
	return 0;
}


