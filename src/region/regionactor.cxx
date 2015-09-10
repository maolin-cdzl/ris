#include <glog/logging.h>
#include "region/regionactor.h"
#include "zmqx/zprotobuf++.h"
#include "zmqx/zloopreader.h"

RIRegionActor::RIRegionActor() :
	m_running(false),
	m_actor(nullptr),
	m_loop(nullptr)
{
}

RIRegionActor::~RIRegionActor() {
	stop();
}

int RIRegionActor::start(const std::shared_ptr<RegionCtx>& ctx) {
	if( m_actor )
		return -1;
	
	m_ctx = ctx;
	m_actor = zactor_new(actorRunner,this);
	if( nullptr == m_actor )
		return -1;
	return 0;
}

int RIRegionActor::stop() {
	if( m_actor ) {
		zactor_destroy(&m_actor);
		m_ctx.reset();
		return 0;
	} else {
		return -1;
	}
}

int RIRegionActor::wait() {
	if( nullptr == m_actor )
		return -1;

	zsock_wait(m_actor);
	stop();
	return 0;
}

std::shared_ptr<Dispatcher> RIRegionActor::make_dispatcher(zsock_t* reader) {
	auto disp = std::make_shared<Dispatcher>();
	disp->set_default(std::bind<int>(&RIRegionActor::defaultOpt,this,reader,std::placeholders::_1));
	disp->register_processer(region::api::HandShake::descriptor(),std::bind<int>(&RIRegionActor::handshake,this,reader,std::placeholders::_1));
	disp->register_processer(region::api::AddService::descriptor(),std::bind<int>(&RIRegionActor::addService,this,reader,std::placeholders::_1));
	disp->register_processer(region::api::RmService::descriptor(),std::bind<int>(&RIRegionActor::rmService,this,reader,std::placeholders::_1));
	disp->register_processer(region::api::AddPayload::descriptor(),std::bind<int>(&RIRegionActor::addPayload,this,reader,std::placeholders::_1));
	disp->register_processer(region::api::RmPayload::descriptor(),std::bind<int>(&RIRegionActor::rmPayload,this,reader,std::placeholders::_1));

	return disp;
}

void RIRegionActor::run(zsock_t* pipe) {
	m_loop = zloop_new();
	zsock_t* rep = zsock_new(ZMQ_REP);

	assert( m_ctx );
	assert(m_loop && rep);
	do {
		LOG(INFO) << "RIRegionActor initialize...";

		m_table = std::make_shared<RIRegionTable>(m_ctx,m_loop);
		m_pub = std::make_shared<RIPublisher>( m_loop );
		m_ssvc = std::make_shared<SnapshotService>( m_loop );

		if( -1 == zsock_bind(rep,"%s",m_ctx->api_address.c_str()) ) {
			LOG(FATAL) << "can not bind Rep on: " << m_ctx->api_address;
			break;
		}
		if( -1 == m_pub->start(m_ctx->pub_address,m_ctx->bind_pub) ) {
			LOG(FATAL) << "can not start pub on: " << m_ctx->pub_address;
			break;
		}
		if( -1 == m_table->start(m_pub) ) {
			LOG(FATAL) << "Start RIRegionTable failed";
			break;
		}
		if( -1 == m_ssvc->start(m_table,m_ctx->snapshot_svc_address,m_ctx->snapshot_worker_address) ) {
			LOG(FATAL) << "Start SnapshotService failed";
			break;
		}

		ZLoopReader pipe_reader(m_loop);
		if( -1 == pipe_reader.start(pipe,std::bind<int>(&RIRegionActor::onPipeReadable,this,std::placeholders::_1)) ) {
			LOG(FATAL) << "Start pipe reader error";
			break;
		}

		auto zdisp = std::make_shared<ZDispatcher>(m_loop);
		if( -1 == zdisp->start(&rep,make_dispatcher(rep)) ) {
			LOG(FATAL) << "Start dispatcher error";
			break;
		}

		zsock_signal(pipe,0);

		m_running = true;
		zsys_interrupted = 0;
		while( m_running ) {
			int result = zloop_start(m_loop);
			if( result == 0 ) {
				LOG(INFO) << "RIRegionActor interrupted";
				m_running = false;
				break;
			}
		}

	} while(0);
	
	m_running = false;
	m_pub.reset();
	m_ssvc.reset();
	m_table.reset();
	if( m_loop ) {
		zloop_destroy(&m_loop);
	}
	if( rep ) {
		zsock_destroy(&rep);
	}
	LOG(INFO) << "RIRegionActor shutdown";
	zsock_signal(pipe,0);
}

void RIRegionActor::actorRunner(zsock_t* pipe,void* args) {
	RIRegionActor* self = (RIRegionActor*)args;
	self->run(pipe);
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
			LOG(INFO) << m_ctx->uuid << " terminated"; 
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


int RIRegionActor::defaultOpt(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg) {
	LOG(WARNING) << "RegionActor Recv unexpected message: " << msg->GetTypeName();
	region::api::Result ret;
	ret.set_result(-1);

	zpb_send(reader,ret);
	return 0;
}

int RIRegionActor::addService(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<region::api::AddService>(msg);
	assert(p);
	region::api::Result result;
	if( 0 == m_table->addService(p->name(),p->address()) ) {
		result.set_result(0);
	} else {
		result.set_result(-1);
	}
	zpb_send(reader,result);
	return 0;
}

int RIRegionActor::rmService(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<region::api::RmService>(msg);
	assert(p);
	region::api::Result result;
	if( 0 == m_table->rmService(p->name()) ) {
		result.set_result(0);
	} else {
		result.set_result(-1);
	}
	zpb_send(reader,result);
	return 0;
}

int RIRegionActor::addPayload(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<region::api::AddPayload>(msg);
	assert(p);
	region::api::Result result;
	if( 0 == m_table->addPayload(p->uuid()) ) {
		result.set_result(0);
	} else {
		result.set_result(-1);
	}

	zpb_send(reader,result);
	return 0;
}

int RIRegionActor::rmPayload(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<region::api::RmPayload>(msg);
	assert(p);
	region::api::Result result;
	if( 0 == m_table->rmPayload(p->uuid()) ) {
		result.set_result(0);
	} else {
		result.set_result(-1);
	}

	zpb_send(reader,result);
	return 0;
}

int RIRegionActor::handshake(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<region::api::HandShake>(msg);
	assert(p);

	zpb_send(reader,*p);
	return 0;
}


