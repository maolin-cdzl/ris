#include <iostream>
#include "ris/region/regionactor.h"
#include <glog/logging.h>
#include <libconfig.h++>
#include "zmqx/zprotobuf++.h"

RIRegionActor::RIRegionActor() :
	m_actor(nullptr),
	m_loop(nullptr),
	m_rep(nullptr),
	m_disp(new Dispatcher())
{
	m_disp->set_member_default(&RIRegionActor::defaultOpt,this);

	m_disp->register_member_processer(region::api::AddService::descriptor(),&RIRegionActor::addService,this);
	m_disp->register_member_processer(region::api::RmService::descriptor(),&RIRegionActor::rmService,this);
	m_disp->register_member_processer(region::api::AddPayload::descriptor(),&RIRegionActor::addPayload,this);
	m_disp->register_member_processer(region::api::RmPayload::descriptor(),&RIRegionActor::rmPayload,this);
}

RIRegionActor::~RIRegionActor() {
	stop();
}

int RIRegionActor::start(const std::string& conf) {
	if( m_actor )
		return -1;

	LOG(INFO) << "RIRegionActor start from config: " << conf;
	if( -1 == loadConfig(conf) )
		return -1;

	m_actor = zactor_new(actorRunner,this);
	if( nullptr == m_actor )
		return -1;
	if( 0 != zsock_wait(m_actor) ) {
		zactor_destroy(&m_actor);
		return -1;
	} else {
		return 0;
	}
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

	std::string regid,idc,msgaddr,ssaddr,workeraddr,pubaddr;

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
		m_region_address = region["localaddress"].c_str();
		msgaddr = region["msgaddress"].c_str();
		pubaddr = region["pubaddress"].c_str();
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

	m_region.id = regid;
	m_region.idc = idc;
	m_region.msg_address = msgaddr;
	m_pub_address = pubaddr;
	if( ! ssaddr.empty() ) {
		m_region.snapshot_address = ssaddr;
		m_snapshot_worker_address = workeraddr;
	}

	return 0;
}

void RIRegionActor::run(zsock_t* pipe) {
	int result = -1;
	m_loop = zloop_new();
	m_rep = zsock_new(ZMQ_REP);

	assert(m_loop && m_rep);
	do {
		LOG(INFO) << "RIRegionActor initialize...";
		m_table = std::make_shared<RIRegionTable>( m_region );
		m_pub = std::make_shared<RIPublisher>( m_loop );
		m_ssvc = std::make_shared<SnapshotService>( m_loop );
		auto zdisp = std::make_shared<ZDispatcher>(m_loop);

		result = -1;
		do {
			if( -1 == zsock_bind(m_rep,"%s",m_region_address.c_str()) ) {
				LOG(ERROR) << "can not bind Rep on: " << m_region_address;
				break;
			}
			if( -1 == m_pub->start(m_table,m_pub_address) )
				break;
			if( -1 == m_ssvc->start(m_table,m_region.snapshot_address,m_snapshot_worker_address) )
				break;

			if( -1 == zloop_reader(m_loop,pipe,pipeReadableAdapter,this) ) {
				LOG(ERROR) << "Register pipe reader error";
				break;
			}

			if( -1 == zdisp->start(m_rep,m_disp) ) {
				LOG(ERROR) << "Start dispatcher error";
				break;
			}
			result = 0;
		} while( 0 );

		zsock_signal(pipe,0);
		if( -1 == result ) {
			LOG(ERROR) << "RIRegionActor initialize error!";
			zsock_signal(pipe,1);
			break;
		} else {
			LOG(INFO) << "RIRegionActor initialize done";
			zsock_signal(pipe,0);
			m_running = true;
		}
		

		while( m_running ) {
			result = zloop_start(m_loop);
			if( result == 0 ) {
				LOG(INFO) << "RIRegionActor interrupted";
				m_running = false;
				break;
			}
		}

	} while(0);

	
	zloop_reader_end(m_loop,pipe);
	m_pub.reset();
	m_ssvc.reset();
	m_table.reset();
	if( m_loop ) {
		zloop_destroy(&m_loop);
	}
	if( m_rep ) {
		zsock_destroy(&m_rep);
	}
	LOG(INFO) << "RIRegionActor shutdown";
}

void RIRegionActor::actorRunner(zsock_t* pipe,void* args) {
	RIRegionActor* self = (RIRegionActor*)args;
	self->run(pipe);
}

int RIRegionActor::pipeReadableAdapter(zloop_t* loop,zsock_t* reader,void* arg) {
	RIRegionActor* self = (RIRegionActor*)arg;
	return self->onPipeReadable(reader);
}


int RIRegionActor::onPipeReadable(zsock_t* pipe) {
	zmsg_t* msg = zmsg_recv(pipe);
	int result = 0;

	do {
		if( nullptr == msg ) {
			LOG(WARNING) << "RIRegionActor recv empty message";
			break;
		} else {

		zframe_t* fr = zmsg_first(msg);
		if( zmsg_size(msg) == 1 && zframe_streq( fr, "$TERM")) {
			LOG(INFO) << m_region.id << " terminated"; 
			m_running = false;
			result = -1;
		} else {
			char* str = zframe_strdup(fr);
			LOG(WARNING) << "RIRegionActor recv unknown message: " << str;
			free(str);
		}
		}
			break;

	} while( 0 );

	if( msg ) {
		zmsg_destroy(&msg);
	}
	return result;
}


void RIRegionActor::defaultOpt(const std::shared_ptr<google::protobuf::Message>& msg,int /*err*/) {
	if( msg ) {
		LOG(WARNING) << "RegionActor Recv unexpected message: " << msg->GetTypeName();
	} else {
		LOG(WARNING) << "RegionActor Recv no protobuf message";
	}
	region::api::Result ret;
	ret.set_result(-1);

	zpb_send(m_rep,ret);
}

void RIRegionActor::addService(const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<region::api::AddService>(msg);
	assert(p);
	region::api::Result result;
	if( 0 == m_table->addService(p->name(),p->address()) ) {
		result.set_result(0);
	} else {
		result.set_result(-1);
	}
	zpb_send(m_rep,result);
}

void RIRegionActor::rmService(const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<region::api::RmService>(msg);
	assert(p);
	region::api::Result result;
	if( 0 == m_table->rmService(p->name()) ) {
		result.set_result(0);
	} else {
		result.set_result(-1);
	}
	zpb_send(m_rep,result);
}

void RIRegionActor::addPayload(const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<region::api::AddPayload>(msg);
	assert(p);
	region::api::Result result;
	if( 0 == m_table->addPayload(p->uuid()) ) {
		result.set_result(0);
	} else {
		result.set_result(-1);
	}

	zpb_send(m_rep,result);
}

void RIRegionActor::rmPayload(const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<region::api::RmPayload>(msg);
	assert(p);
	region::api::Result result;
	if( 0 == m_table->rmPayload(p->uuid()) ) {
		result.set_result(0);
	} else {
		result.set_result(-1);
	}

	zpb_send(m_rep,result);
}


