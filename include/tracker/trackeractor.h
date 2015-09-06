#pragma once

#include <memory>
#include <string>
#include <czmq.h>

#include "zmqx/zdispatcher.h"

#include "tracker/trackerctx.h"
#include "tracker/trackertable.h"
#include "tracker/subscriber.h"
#include "snapshot/snapshotservice.h"
#include "tracker/fromregionfactory.h"


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
	void onFactoryDone(int err,const std::shared_ptr<TrackerFactoryProduct>& product);

	std::shared_ptr<Dispatcher> make_dispatcher(zsock_t* reader);
	static void actorRunner(zsock_t* pipe,void* args);
private:
	void defaultOpt(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg,int err);
	void onHandShake(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg);
	void onStaticsReq(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg);
	void onRegionReq(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg);
	void onServiceRouteReq(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg);
	void onPayloadRouteReq(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg);
	void onPayloadsRouteReq(zsock_t* reader,const std::shared_ptr<google::protobuf::Message>& msg);
private:
	bool					m_running;
	zactor_t*				m_actor;
	zloop_t*				m_loop;

	std::shared_ptr<TrackerCtx>				m_ctx;
	std::shared_ptr<RITrackerTable>			m_table;
	std::shared_ptr<RISubscriber>			m_sub;
	std::shared_ptr<SnapshotService>		m_ssvc;
	std::shared_ptr<FromRegionFactory>		m_factory;
};

