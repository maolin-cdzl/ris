#pragma once

#include <memory>
#include <string>
#include <czmq.h>

#include "zmqx/zpbreader.h"

#include "tracker/trackerctx.h"
#include "tracker/trackertable.h"
#include "tracker/subscriber.h"
#include "snapshot/snapshotservice.h"
#include "tracker/pubtracker.h"


class RITrackerActor {
public:
	RITrackerActor();
	~RITrackerActor();

	int start(const std::shared_ptr<TrackerCtx>& ctx);
	void stop();

	// wait actor shutdown,it will block caller
	int wait();

	inline std::shared_ptr<TrackerCtx> getCtx() const {
		return m_ctx;
	}

private:
	void run(zsock_t* pipe);
	int onPipeReadable(zsock_t* pipe);

	std::shared_ptr<envelope_dispatcher_t> make_dispatcher();
	static void actorRunner(zsock_t* pipe,void* args);
private:
	int defaultOpt(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,std::unique_ptr<ZEnvelope>& envelope);
	int onHandShake(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,std::unique_ptr<ZEnvelope>& envelope);
	int onStaticsReq(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,std::unique_ptr<ZEnvelope>& envelope);
	int onRegionReq(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,std::unique_ptr<ZEnvelope>& envelope);
	int onServiceRouteReq(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,std::unique_ptr<ZEnvelope>& envelope);
	int onPayloadRouteReq(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,std::unique_ptr<ZEnvelope>& envelope);
	int onPayloadsRouteReq(const std::shared_ptr<google::protobuf::Message>& msg,zsock_t* sock,std::unique_ptr<ZEnvelope>& envelope);
private:
	bool					m_running;
	zactor_t*				m_actor;
	zloop_t*				m_loop;

	std::shared_ptr<TrackerCtx>				m_ctx;
	std::shared_ptr<RITrackerTable>			m_table;
	std::shared_ptr<RISubscriber>			m_sub;
	std::shared_ptr<PubTracker>				m_tracker;
};

