#include <glog/logging.h>
#include "tracker/trackeractor.h"
#include "ris/trackerapi.pb.h"
#include "snapshot/snapshotfeature.h"
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

std::shared_ptr<envelope_dispatcher_t> RITrackerActor::make_dispatcher() {
	auto disp = std::make_shared<envelope_dispatcher_t>();
	disp->set_default(std::bind<int>(&RITrackerActor::defaultOpt,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
	disp->register_processer(tracker::api::HandShake::descriptor(),std::bind<int>(&RITrackerActor::onHandShake,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
	disp->register_processer(tracker::api::StatisticsReq::descriptor(),std::bind<int>(&RITrackerActor::onStaticsReq,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
	disp->register_processer(tracker::api::RegionReq::descriptor(),std::bind<int>(&RITrackerActor::onRegionReq,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
	disp->register_processer(tracker::api::ServiceRouteReq::descriptor(),std::bind<int>(&RITrackerActor::onServiceRouteReq,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
	disp->register_processer(tracker::api::PayloadRouteReq::descriptor(),std::bind<int>(&RITrackerActor::onPayloadRouteReq,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
	disp->register_processer(tracker::api::PayloadsRouteReq::descriptor(),std::bind<int>(&RITrackerActor::onPayloadsRouteReq,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
	return std::move(disp);
}

void RITrackerActor::run(zsock_t* pipe) {
	CHECK( m_ctx );

	m_loop = zloop_new();
	CHECK_NOTNULL(m_loop);

	m_running = true;
	zsock_t* rep = nullptr;
	do {
		ZLoopReader pipe_reader(m_loop);
		if( -1 == pipe_reader.start(pipe,std::bind<int>(&RITrackerActor::onPipeReadable,this,std::placeholders::_1)) ) {
			LOG(FATAL) << "Register pipe reader error";
			break;
		}

		rep = zsock_new(ZMQ_ROUTER);
		if( -1 == zsock_bind(rep,"%s",m_ctx->api_address.c_str()) ) {
			LOG(FATAL) << "Error when binding rep socket to: " << m_ctx->api_address;
			break;
		}

		auto rep_reader = make_zpb_reader(m_loop,&rep,make_dispatcher());
		CHECK(rep_reader) << "Start zdispatcher failed";

		m_table = std::make_shared<RITrackerTable>(m_loop);
		if( -1 == m_table->start() ) {
			LOG(FATAL) << "Start table failed";
			break;
		}

		m_tracker = std::make_shared<PubTracker>(m_loop);
		if( -1 == m_tracker->start(m_table) ) {
			LOG(FATAL) << "Start subscriber failed";
		}

		m_table->setNextHandler(m_tracker);

		m_sub = std::make_shared<RISubscriber>(m_loop);
		if( -1 == m_sub->start(m_ctx->pub_address,m_table) ) {
			LOG(FATAL) << "Start subscriber failed";
			break;
		}
		
		auto feature = std::make_shared<SnapshotFeature>(m_loop);
		if( -1 == feature->start(m_table) ) {
			LOG(FATAL) << "Start SnapshotFeature failed";
			break;
		}

		auto ssvc = std::make_shared<SnapshotService>();
		if( -1 == ssvc->start(feature,m_ctx->snapshot_address) ) {
			LOG(FATAL) << "Tracker start snapshot service failed";
			break;
		}

		zsock_signal(pipe,0);
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
	if( m_tracker ) {
		m_tracker.reset();
	}

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
	CHECK_EQ(1,zmsg_size(msg));
	CHECK(zframe_streq(zmsg_first(msg),"$TERM"));
#endif

	if( msg ) {
		zmsg_destroy(&msg);
	}
	m_running = false;
	return -1;
}

void RITrackerActor::actorRunner(zsock_t* pipe,void* args) {
	RITrackerActor* self = (RITrackerActor*)args;
	self->run(pipe);
}

int RITrackerActor::defaultOpt(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,const std::shared_ptr<ZEnvelope>& envelope) {
	(void)sock;
	(void)envelope;
	LOG(WARNING) << "TrackerActor Recv unexpected message: " << msg->GetTypeName();
	return 0;
}

int RITrackerActor::onHandShake(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,const std::shared_ptr<ZEnvelope>& envelope) {
	auto p = std::dynamic_pointer_cast<tracker::api::HandShake>(msg);
	zpb_send(sock,envelope,*p);
	return 0;
}

int RITrackerActor::onStaticsReq(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,const std::shared_ptr<ZEnvelope>& envelope) {
	auto p = std::dynamic_pointer_cast<tracker::api::StatisticsReq>(msg);
	tracker::api::StatisticsRep rep;

	auto regions = m_table->regions();
	auto services = m_table->services();

	for(auto it=regions.begin(); it != regions.end(); ++it) {
		(*it)->toProtobuf(rep.add_regions());
	}
	std::copy(services.begin(),services.end(),google::protobuf::RepeatedFieldBackInserter(rep.mutable_services()));
	rep.set_payload_count( m_table->payload_size() );
	zpb_send(sock,envelope,rep);
	return 0;
}

int RITrackerActor::onRegionReq(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,const std::shared_ptr<ZEnvelope>& envelope) {
	auto p = std::dynamic_pointer_cast<tracker::api::RegionReq>(msg);
	tracker::api::RegionRep rep;
	auto region = m_table->getRegion(p->uuid());
	if( region ) {
		region->toProtobuf( rep.mutable_region() );
	}
	zpb_send(sock,envelope,rep);
	return 0;
}

int RITrackerActor::onServiceRouteReq(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,const std::shared_ptr<ZEnvelope>& envelope) {
	auto p = std::dynamic_pointer_cast<tracker::api::ServiceRouteReq>(msg);
	tracker::api::ServiceRouteRep rep;
	
	auto ri = rep.mutable_route();
	ri->set_target(p->svc());
	auto result = m_table->robinRouteService( p->svc() );
	if( result.first ) {
		result.second.toProtobuf(ri->mutable_endpoint());
	}
	zpb_send(sock,envelope,rep);
	return 0;
}

int RITrackerActor::onPayloadRouteReq(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,const std::shared_ptr<ZEnvelope>& envelope) {
	auto p = std::dynamic_pointer_cast<tracker::api::PayloadRouteReq>(msg);
	tracker::api::PayloadRouteRep rep;

	auto ri = rep.mutable_route();
	ri->set_target(p->payload());
	auto result = m_table->routePayload(p->payload());
	if( result.first ) {
		result.second.toProtobuf(ri->mutable_endpoint());
	}
	zpb_send(sock,envelope,rep);
	return 0;
}

int RITrackerActor::onPayloadsRouteReq(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,const std::shared_ptr<ZEnvelope>& envelope) {
	auto p = std::dynamic_pointer_cast<tracker::api::PayloadsRouteReq>(msg);
	tracker::api::PayloadsRouteRep rep;

	for(size_t i=0; i < (size_t)p->payloads_size(); ++i) {
		auto& payload = p->payloads(i);
		auto ri = rep.add_routes();
		ri->set_target(payload);

		auto result = m_table->routePayload(payload);
		if( result.first ) {
			result.second.toProtobuf(ri->mutable_endpoint());
		}
	}

	zpb_send(sock,envelope,rep);
	return 0;
}

