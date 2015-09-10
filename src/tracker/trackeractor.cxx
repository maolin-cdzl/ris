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

std::shared_ptr<Dispatcher> RITrackerActor::make_dispatcher(zsock_t* reader) {
	auto disp = std::make_shared<Dispatcher>();
	disp->set_default(std::bind<int>(&RITrackerActor::defaultOpt,this,reader,std::placeholders::_1));
	disp->register_processer(tracker::api::HandShake::descriptor(),std::bind<int>(&RITrackerActor::onHandShake,this,reader,std::placeholders::_1));
	disp->register_processer(tracker::api::StatisticsReq::descriptor(),std::bind<int>(&RITrackerActor::onStaticsReq,this,reader,std::placeholders::_1));
	disp->register_processer(tracker::api::RegionReq::descriptor(),std::bind<int>(&RITrackerActor::onRegionReq,this,reader,std::placeholders::_1));
	disp->register_processer(tracker::api::ServiceRouteReq::descriptor(),std::bind<int>(&RITrackerActor::onServiceRouteReq,this,reader,std::placeholders::_1));
	disp->register_processer(tracker::api::PayloadRouteReq::descriptor(),std::bind<int>(&RITrackerActor::onPayloadRouteReq,this,reader,std::placeholders::_1));
	disp->register_processer(tracker::api::PayloadsRouteReq::descriptor(),std::bind<int>(&RITrackerActor::onPayloadsRouteReq,this,reader,std::placeholders::_1));
	return disp;
}

void RITrackerActor::run(zsock_t* pipe) {
	assert( m_ctx );
	
	zsock_t* rep = nullptr;
	do {
		LOG(INFO) << "RITrackerActor initialize...";
		m_loop = zloop_new();
		assert(m_loop);
		
		ZLoopReader pipe_reader(m_loop);
		if( -1 == pipe_reader.start(pipe,std::bind<int>(&RITrackerActor::onPipeReadable,this,std::placeholders::_1)) ) {
			LOG(FATAL) << "Register pipe reader error";
			break;
		}

		{
			auto factory = std::make_shared<FromRegionFactory>(m_loop);
			if( -1 == factory->start(m_ctx->pub_address,std::bind(&RITrackerActor::onFactoryDone,this,std::placeholders::_1,std::placeholders::_2),m_ctx->factory_timeout) ) {
				LOG(FATAL) << "Start factory failed";
				break;
			}
			zsock_signal(pipe,0);

			m_running = true;
			while( m_running ) {
				if( 0 == zloop_start(m_loop) ) {
					LOG(INFO) << "RITrackerActor interrupted";
					break;
				}
			}
		}

		if( 0 != zsys_interrupted ) {
			break;
		}

		m_ssvc = std::make_shared<SnapshotService>(m_loop);
		if( -1 == m_ssvc->start(m_table,m_ctx->snapshot_svc_address,m_ctx->snapshot_worker_address) ) {
			LOG(FATAL) << "Tracker start snapshot service failed";
			break;
		}

		rep = zsock_new(ZMQ_REP);
		if( -1 == zsock_bind(rep,"%s",m_ctx->api_address.c_str()) ) {
			LOG(FATAL) << "Error when binding rep socket to: " << m_ctx->api_address;
			break;
		}
		auto zdisp = std::make_shared<ZDispatcher>(m_loop);
		if( -1 == zdisp->start(&rep,make_dispatcher(rep)) ) {
			LOG(FATAL) << "Start zdispatcher failed";
			break;
		}
		
		m_running = true;
		while( m_running ) {
			if(  0 == zloop_start(m_loop) ) {
				LOG(INFO) << "RITrackerActor interrupted";
				break;
			}
		}
	} while(0);

	m_running = false;

	if( m_ssvc )
		m_ssvc.reset();
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
	zsock_signal(pipe,0);
}

int RITrackerActor::onPipeReadable(zsock_t* pipe) {
	zmsg_t* msg = zmsg_recv(pipe);
	int result = 0;

	do {
		if( nullptr == msg ) {
			LOG(WARNING) << "RITrackerActor recv empty message";
			break;
		} else {

		zframe_t* fr = zmsg_first(msg);
		if( zmsg_size(msg) == 1 && zframe_streq( fr, "$TERM")) {
			LOG(INFO) << "Tracker terminated"; 
			m_running = false;
			result = -1;
		} else {
			char* str = zframe_strdup(fr);
			LOG(WARNING) << "RITrackerActor recv unknown message: " << str;
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

void RITrackerActor::actorRunner(zsock_t* pipe,void* args) {
	RITrackerActor* self = (RITrackerActor*)args;
	self->run(pipe);
}

int RITrackerActor::defaultOpt(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg) {
	LOG(WARNING) << "TrackerActor Recv unexpected message: " << msg->GetTypeName();
	tracker::api::Error ret;
	ret.set_code(-1);

	zpb_send(reader,ret);
	return 0;
}

int RITrackerActor::onHandShake(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<tracker::api::HandShake>(msg);
	zpb_send(reader,*p);
	return 0;
}

int RITrackerActor::onStaticsReq(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<tracker::api::StatisticsReq>(msg);
	tracker::api::StatisticsRep rep;
	rep.set_region_count( m_table->region_size() );
	rep.set_service_count( m_table->service_size() );
	rep.set_payload_count( m_table->payload_size() );
	zpb_send(reader,rep);
	return 0;
}

int RITrackerActor::onRegionReq(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg) {
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
	zpb_send(reader,rep);
	return 0;
}

int RITrackerActor::onServiceRouteReq(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<tracker::api::ServiceRouteReq>(msg);
	tracker::api::ServiceRouteRep rep;
	
	auto ri = rep.mutable_route();
	ri->set_target(p->svc());
	auto result = m_table->robinRouteService( p->svc() );
	if( result.first ) {
		ri->set_region(result.first->id);
		ri->set_address(result.second);
	}
	zpb_send(reader,rep);
	return 0;
}

int RITrackerActor::onPayloadRouteReq(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg) {
	auto p = std::dynamic_pointer_cast<tracker::api::PayloadRouteReq>(msg);
	tracker::api::PayloadRouteRep rep;

	auto ri = rep.mutable_route();
	ri->set_target(p->payload());
	auto region = m_table->routePayload(p->payload());
	if( region ) {
		ri->set_region(region->id);
		ri->set_address(region->bus_address);
	}
	zpb_send(reader,rep);
	return 0;
}

int RITrackerActor::onPayloadsRouteReq(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg) {
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

	zpb_send(reader,rep);
	return 0;
}


void RITrackerActor::onFactoryDone(int err,const std::shared_ptr<TrackerFactoryProduct>& product) {
	if( 0 == err && product ) {
		m_table = product->table;
		m_sub = product->sub;
		assert( m_table );
		assert( m_sub );

		LOG(INFO) << "Factory done success,region: " << m_table->region_size() << ", service: " << m_table->service_size() << ", payloads: " << m_table->payload_size();

	} else {
		LOG(FATAL) << "Factory product error: " << err;
	}
	m_running = false;
}

