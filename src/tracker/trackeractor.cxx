#include <glog/logging.h>
#include "tracker/trackeractor.h"
#include "ris/trackerapi.pb.h"
#include "zmqx/zprotobuf++.h"

RITrackerActor::RITrackerActor() :
	m_running(false),
	m_actor(nullptr),
	m_loop(nullptr)
{
}

RITrackerActor::~RITrackerActor() {
	stop();
}

int RITrackerActor::start(const std::shared_ptr<TrackerCtx>& ctx) {
	if( m_actor )
		return -1;

	m_ctx = ctx;
	m_actor = zactor_new(actorRunner,this);
	if( nullptr == m_actor )
		return -1;
	return 0;
}

void RITrackerActor::stop() {
	if( m_actor ) {
		zactor_destroy(&m_actor);
		m_ctx.reset();
	}
}

int RITrackerActor::wait() {
	if( nullptr == m_actor )
		return -1;
	zsock_wait(m_actor);
	stop();
	return 0;
}

std::shared_ptr<Dispatcher> RITrackerActor::make_dispatcher(ZDispatcher& zdisp) {
	auto disp = std::make_shared<Dispatcher>();
	disp->set_default(std::bind<int>(&RITrackerActor::defaultOpt,this,std::ref(zdisp),std::placeholders::_1));
	disp->register_processer(tracker::api::HandShake::descriptor(),std::bind<int>(&RITrackerActor::onHandShake,this,std::ref(zdisp),std::placeholders::_1));
	disp->register_processer(tracker::api::StatisticsReq::descriptor(),std::bind<int>(&RITrackerActor::onStaticsReq,this,std::ref(zdisp),std::placeholders::_1));
	disp->register_processer(tracker::api::RegionReq::descriptor(),std::bind<int>(&RITrackerActor::onRegionReq,this,std::ref(zdisp),std::placeholders::_1));
	disp->register_processer(tracker::api::ServiceRouteReq::descriptor(),std::bind<int>(&RITrackerActor::onServiceRouteReq,this,std::ref(zdisp),std::placeholders::_1));
	disp->register_processer(tracker::api::PayloadRouteReq::descriptor(),std::bind<int>(&RITrackerActor::onPayloadRouteReq,this,std::ref(zdisp),std::placeholders::_1));
	disp->register_processer(tracker::api::PayloadsRouteReq::descriptor(),std::bind<int>(&RITrackerActor::onPayloadsRouteReq,this,std::ref(zdisp),std::placeholders::_1));
	return disp;
}

int RITrackerActor::initialize(zsock_t* pipe) {
	int result = 0;
	LOG(INFO) << "RITrackerActor initialize...";
	do {
		auto factory = std::make_shared<FromRegionFactory>(m_loop);
		if( -1 == factory->start(m_ctx->pub_address,std::bind(&RITrackerActor::onFactoryDone,this,&result,std::placeholders::_1,std::placeholders::_2),m_ctx->factory_timeout) ) {
			LOG(FATAL) << "Start factory failed";
			break;
		}

		while( 0 == result ) {
			if( 0 == zloop_start(m_loop) ) {
				LOG(INFO) << "RITrackerActor interrupted when track snapshot";
				break;
			}
		}
	} while(0);

	zsock_signal(pipe,0);
	return (result == 1 ? 0 : -1);
}

void RITrackerActor::run(zsock_t* pipe) {
	assert( m_ctx );

	m_loop = zloop_new();
	assert(m_loop);

	m_running = true;
	zsock_t* rep = nullptr;
	do {
		if( -1 == initialize(pipe) ) {
			LOG(ERROR) << "RITrackerActor initialize failed";
			break;
		}
		
		ZLoopReader pipe_reader(m_loop);
		if( -1 == pipe_reader.start(pipe,std::bind<int>(&RITrackerActor::onPipeReadable,this,std::placeholders::_1)) ) {
			LOG(FATAL) << "Register pipe reader error";
			break;
		}

		auto ssvc = std::make_shared<SnapshotService>(m_loop);
		if( -1 == ssvc->start(m_table,m_ctx->snapshot_svc_address,m_ctx->snapshot_worker_address) ) {
			LOG(FATAL) << "Tracker start snapshot service failed";
			break;
		}

		rep = zsock_new(ZMQ_ROUTER);
		if( -1 == zsock_bind(rep,"%s",m_ctx->api_address.c_str()) ) {
			LOG(FATAL) << "Error when binding rep socket to: " << m_ctx->api_address;
			break;
		}
		auto zdisp = std::make_shared<ZDispatcher>(m_loop);
		if( -1 == zdisp->start(&rep,make_dispatcher(*zdisp),true) ) {
			LOG(FATAL) << "Start zdispatcher failed";
			break;
		}
		
		while( m_running ) {
			if(  0 == zloop_start(m_loop) ) {
				LOG(INFO) << "RITrackerActor interrupted";
				break;
			}
		}
	} while(0);

	m_running = false;

	if( m_sub )
		m_sub.reset();
	if( m_table )
		m_table.reset();

	if( rep ) {
		zsock_destroy(&rep);
	}
	if( m_loop ) {
		zloop_destroy(&m_loop);
	}
	LOG(INFO) << "RITrackerActor shutdown";
}

int RITrackerActor::onPipeReadable(zsock_t* pipe) {
	zmsg_t* msg = zmsg_recv(pipe);
	DLOG(INFO) << "TrackerActor interrupte by pipe command";
#ifndef NDEBUG
	assert(1 == zmsg_size(msg));
	assert(zframe_streq(zmsg_first(msg),"$TERM"));
#endif

	if( msg ) {
		zmsg_destroy(&msg);
	}
	return -1;
}

void RITrackerActor::actorRunner(zsock_t* pipe,void* args) {
	RITrackerActor* self = (RITrackerActor*)args;
	self->run(pipe);
}

int RITrackerActor::defaultOpt(ZDispatcher&,const std::shared_ptr<google::protobuf::Message>& msg) {
	LOG(WARNING) << "TrackerActor Recv unexpected message: " << msg->GetTypeName();
	return 0;
}

int RITrackerActor::onHandShake(ZDispatcher& zdisp,const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<tracker::api::HandShake>(msg);
	zdisp.sendback(*p);
	return 0;
}

int RITrackerActor::onStaticsReq(ZDispatcher& zdisp,const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<tracker::api::StatisticsReq>(msg);
	tracker::api::StatisticsRep rep;
	rep.set_region_count( m_table->region_size() );
	rep.set_service_count( m_table->service_size() );
	rep.set_payload_count( m_table->payload_size() );
	zdisp.sendback(rep);
	return 0;
}

int RITrackerActor::onRegionReq(ZDispatcher& zdisp,const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<tracker::api::RegionReq>(msg);
	tracker::api::RegionRep rep;
	auto region = m_table->getRegion(p->uuid());
	if( region ) {
		auto pr = rep.mutable_region();
		pr->set_uuid( region->id );
		pr->set_version( region->version );
		pr->set_idc( region->idc );
		pr->set_bus_address( region->bus_address );
		pr->set_snapshot_address( region->snapshot_address );
	}
	zdisp.sendback(rep);
	return 0;
}

int RITrackerActor::onServiceRouteReq(ZDispatcher& zdisp,const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<tracker::api::ServiceRouteReq>(msg);
	tracker::api::ServiceRouteRep rep;
	
	auto ri = rep.mutable_route();
	ri->set_target(p->svc());
	auto result = m_table->robinRouteService( p->svc() );
	if( result.first ) {
		ri->set_region(result.first->id);
		ri->set_address(result.second);
	}
	zdisp.sendback(rep);
	return 0;
}

int RITrackerActor::onPayloadRouteReq(ZDispatcher& zdisp,const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<tracker::api::PayloadRouteReq>(msg);
	tracker::api::PayloadRouteRep rep;

	auto ri = rep.mutable_route();
	ri->set_target(p->payload());
	auto region = m_table->routePayload(p->payload());
	if( region ) {
		ri->set_region(region->id);
		ri->set_address(region->bus_address);
	}
	zdisp.sendback(rep);
	return 0;
}

int RITrackerActor::onPayloadsRouteReq(ZDispatcher& zdisp,const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<tracker::api::PayloadsRouteReq>(msg);
	tracker::api::PayloadsRouteRep rep;

	for(size_t i=0; i < (size_t)p->payloads_size(); ++i) {
		auto& payload = p->payloads(i);
		auto ri = rep.add_routes();
		ri->set_target(payload);

		auto region = m_table->routePayload(payload);
		if( region ) {
			ri->set_region(region->id);
			ri->set_address(region->bus_address);
		}
	}

	zdisp.sendback(rep);
	return 0;
}


void RITrackerActor::onFactoryDone(int* result,int err,const std::shared_ptr<TrackerFactoryProduct>& product) {
	if( 0 == err && product ) {
		m_table = product->table;
		m_sub = product->sub;
		assert( m_table );
		assert( m_sub );
		*result = 1;
		LOG(INFO) << "Factory done success,region: " << m_table->region_size() << ", service: " << m_table->service_size() << ", payloads: " << m_table->payload_size();

	} else {
		*result = -1;
		LOG(FATAL) << "Factory product error: " << err;
	}
}

