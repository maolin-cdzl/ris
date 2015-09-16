#include <glog/logging.h>
#include "snapshot/snapshotfeature.h"
#include "region/regionactor.h"
#include "zmqx/zhelper.h"
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

std::shared_ptr<envelope_dispatcher_t> RIRegionActor::make_dispatcher() {
	auto disp = std::make_shared<envelope_dispatcher_t>();
	disp->set_default(std::bind<int>(&RIRegionActor::defaultOpt,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
	disp->register_processer(region::api::HandShake::descriptor(),std::bind<int>(&RIRegionActor::handshake,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
	disp->register_processer(region::api::AddService::descriptor(),std::bind<int>(&RIRegionActor::addService,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
	disp->register_processer(region::api::RmService::descriptor(),std::bind<int>(&RIRegionActor::rmService,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
	disp->register_processer(region::api::AddPayload::descriptor(),std::bind<int>(&RIRegionActor::addPayload,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
	disp->register_processer(region::api::RmPayload::descriptor(),std::bind<int>(&RIRegionActor::rmPayload,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));

	return std::move(disp);
}

void RIRegionActor::run(zsock_t* pipe) {
	CHECK( m_ctx );

	m_loop = zloop_new();
	CHECK_NOTNULL(m_loop);

	zsock_t* rep = nullptr;

	do {
		LOG(INFO) << "RIRegionActor initialize...";

		auto pub = std::make_shared<RIPublisher>( m_loop );
		if( -1 == pub->start(m_ctx->pub_address,m_ctx->bind_pub) ) {
			LOG(FATAL) << "can not start pub on: " << m_ctx->pub_address;
			break;
		}

		m_table = std::make_shared<RIRegionTable>(m_ctx,m_loop);
		if( -1 == m_table->start(pub) ) {
			LOG(FATAL) << "Start RIRegionTable failed";
			break;
		}

		auto feature = std::make_shared<SnapshotFeature>(m_loop);
		if( -1 == feature->start(m_table) ) {
			LOG(FATAL) << "Start SnapshotFeature failed";
			break;
		}

		auto ssvc = std::make_shared<SnapshotService>();
		if( -1 == ssvc->start(feature,m_ctx->snapshot_address) ) {
			LOG(FATAL) << "Start SnapshotService failed";
			break;
		}

		ZLoopReader pipe_reader(m_loop);
		if( -1 == pipe_reader.start(pipe,std::bind<int>(&RIRegionActor::onPipeReadable,this,std::placeholders::_1)) ) {
			LOG(FATAL) << "Start pipe reader error";
			break;
		}

		rep = zsock_new(ZMQ_ROUTER);
		if( -1 == zsock_bind(rep,"%s",m_ctx->api_address.c_str()) ) {
			LOG(FATAL) << "can not bind Rep on: " << m_ctx->api_address;
			break;
		}

		auto rep_reader = make_zpb_reader(m_loop,&rep,make_dispatcher());
		CHECK(rep_reader) << "Start Rep reader error";

		zsock_signal(pipe,0);

		m_running = true;
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


int RIRegionActor::defaultOpt(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,std::unique_ptr<ZEnvelope>& envelope) {
	(void)sock;
	(void)envelope;
	LOG(WARNING) << "RegionActor Recv unexpected message: " << msg->GetTypeName();
	return 0;
}

int RIRegionActor::addService(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,std::unique_ptr<ZEnvelope>& envelope) {
	auto p = std::dynamic_pointer_cast<region::api::AddService>(msg);
	CHECK(p);
	int err = m_table->addService(p->name(),p->address());

	if( p->rep() ) {
		region::api::Result result;
		result.set_result(err);
		result.set_version(m_table->version());
		zpb_send(sock,std::move(envelope),result);
	}
	return 0;
}

int RIRegionActor::rmService(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,std::unique_ptr<ZEnvelope>& envelope) {
	auto p = std::dynamic_pointer_cast<region::api::RmService>(msg);
	CHECK(p);

	int err = m_table->rmService(p->name());

	if( p->rep() ) {
		region::api::Result result;
		result.set_result(err);
		result.set_version(m_table->version());
		zpb_send(sock,std::move(envelope),result);
	}
	return 0;
}

int RIRegionActor::addPayload(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,std::unique_ptr<ZEnvelope>& envelope) {
	auto p = std::dynamic_pointer_cast<region::api::AddPayload>(msg);
	CHECK(p);
	int err = m_table->addPayload(p->uuid());

	if( p->rep() ) {
		region::api::Result result;
		result.set_result(err);
		result.set_version(m_table->version());
		zpb_send(sock,std::move(envelope),result);
	}
	return 0;
}

int RIRegionActor::rmPayload(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,std::unique_ptr<ZEnvelope>& envelope) {
	auto p = std::dynamic_pointer_cast<region::api::RmPayload>(msg);
	CHECK(p);
	int err = m_table->rmPayload(p->uuid());

	if( p->rep() ) {
		region::api::Result result;
		result.set_result(err);
		result.set_version(m_table->version());
		zpb_send(sock,std::move(envelope),result);
	}
	return 0;
}

int RIRegionActor::handshake(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,std::unique_ptr<ZEnvelope>& envelope) {
	auto p = std::dynamic_pointer_cast<region::api::HandShake>(msg);
	CHECK(p);

	p->set_version(m_table->version());
	zpb_send(sock,std::move(envelope),*p);
	return 0;
}


