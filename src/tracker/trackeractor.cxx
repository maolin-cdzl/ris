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
	disp->set_default(std::bind(&RITrackerActor::defaultOpt,this,reader,std::placeholders::_1,std::placeholders::_2));
	return disp;
}

void RITrackerActor::run(zsock_t* pipe) {
	assert( m_ctx );
	
	zsock_t* rep = nullptr;
	do {
		LOG(INFO) << "RITrackerActor initialize...";
		m_loop = zloop_new();
		assert(m_loop);
		rep = zsock_new_rep(m_ctx->api_address.c_str());
		if( nullptr == rep ) {
			LOG(FATAL) << "Create rep socket failed";
			break;
		}
		
		ZLoopReader pipe_reader(m_loop);
		if( -1 == pipe_reader.start(pipe,std::bind<int>(&RITrackerActor::onPipeReadable,this,std::placeholders::_1)) ) {
			LOG(FATAL) << "Register pipe reader error";
			break;
		}

		auto zdisp = std::make_shared<ZDispatcher>(m_loop);
		if( -1 == zdisp->start(&rep,make_dispatcher(rep)) ) {
			LOG(FATAL) << "Start zdispatcher failed";
			break;
		}
		
		m_factory = std::make_shared<FromRegionFactory>(m_loop);
		if( -1 == m_factory->start(m_ctx->pub_address,std::bind(&RITrackerActor::onFactoryDone,this,std::placeholders::_1,std::placeholders::_2)) ) {
			LOG(FATAL) << "Start factory failed";
			break;
		}
		m_running = true;
		zsock_signal(pipe,0);
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

	if( m_factory )
		m_factory.reset();
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

void RITrackerActor::defaultOpt(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg,int) {
	if( msg ) {
		LOG(WARNING) << "TrackerActor Recv unexpected message: " << msg->GetTypeName();
	} else {
		LOG(WARNING) << "TrackerActor Recv no protobuf message";
	}
	tracker::api::Result ret;
	ret.set_result(-1);

	zpb_send(reader,ret);
}


void RITrackerActor::onFactoryDone(int err,const std::shared_ptr<TrackerFactoryProduct>& product) {
	if( 0 == err && product ) {
		m_table = product->table;
		m_sub = product->sub;
		assert( m_table );
		assert( m_sub );
		m_factory.reset();

		LOG(INFO) << "Factory done success,region: " << m_table->region_size() << ", service: " << m_table->service_size() << ", payloads: " << m_table->payload_size();

		m_ssvc = std::make_shared<SnapshotService>(m_loop);
		if( -1 == m_ssvc->start(m_table,m_ctx->snapshot_svc_address,m_ctx->snapshot_worker_address) ) {
			LOG(FATAL) << "Tracker start snapshot service failed";
			zsys_interrupted = -1;
			m_running = false;
		}
	} else {
		LOG(FATAL) << "Factory product error: " << err;
		zsys_interrupted = -1;
		m_running = false;
	}
}

