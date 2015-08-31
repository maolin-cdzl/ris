#include <glog/logging.h>
#include "ris/tracker/trackeractor.h"
#include "ris/trackerapi.pb.h"
#include "zmqx/zprotobuf++.h"

RITrackerActor::RITrackerActor() :
	m_running(false),
	m_actor(nullptr),
	m_loop(nullptr),
	m_rep(nullptr),
	m_disp(new Dispatcher())
{
	m_disp->set_member_default(&RITrackerActor::defaultOpt,this);
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
	if( 0 != zsock_wait(m_actor) ) {
		zactor_destroy(&m_actor);
		return -1;
	} else {
		return 0;
	}
}

void RITrackerActor::stop() {
	if( m_actor ) {
		zactor_destroy(&m_actor);
		m_ctx.reset();
	}
}


void RITrackerActor::run(zsock_t* pipe) {
	assert( m_ctx );

	do {
		m_loop = zloop_new();
		assert(m_loop);
		m_rep = zsock_new_rep(m_ctx->api_address.c_str());
		if( nullptr == m_rep )
			break;

		if( -1 == zloop_reader(m_loop,pipe,pipeReadableAdapter,this) ) {
			LOG(ERROR) << "Register pipe reader error";
			break;
		}

		auto zdisp = std::make_shared<ZDispatcher>(m_loop);
		if( -1 == zdisp->start(m_rep,m_disp) ) {
			break;
		}
		
		m_factory = std::make_shared<FromRegionFactory>(m_loop);
		if( -1 == m_factory->start(m_ctx->pub_address,std::bind(&RITrackerActor::onFactoryDone,this,std::placeholders::_1,std::placeholders::_2)) ) {
			break;
		}
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

	zloop_reader_end(m_loop,pipe);

	if( m_factory )
		m_factory.reset();
	if( m_ssvc )
		m_ssvc.reset();
	if( m_sub )
		m_sub.reset();
	if( m_table )
		m_table.reset();

	if( m_rep ) {
		zsock_destroy(&m_rep);
	}
	if( m_loop ) {
		zloop_destroy(&m_loop);
	}
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

int RITrackerActor::pipeReadableAdapter(zloop_t* loop,zsock_t* reader,void* arg) {
	(void)loop;
	RITrackerActor* self = (RITrackerActor*)arg;
	return self->onPipeReadable(reader);
}


void RITrackerActor::defaultOpt(const std::shared_ptr<google::protobuf::Message>& msg,int err) {
	if( msg ) {
		LOG(WARNING) << "TrackerActor Recv unexpected message: " << msg->GetTypeName();
	} else {
		LOG(WARNING) << "TrackerActor Recv no protobuf message";
	}
	tracker::api::Result ret;
	ret.set_result(-1);

	zpb_send(m_rep,ret);
}


void RITrackerActor::onFactoryDone(int err,const std::shared_ptr<TrackerFactoryProduct>& product) {
	if( 0 == err && product ) {
		LOG(INFO) << "Factory done success";
		m_table = product->table;
		m_sub = product->sub;
		assert( m_table );
		assert( m_sub );

		m_factory.reset();

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

