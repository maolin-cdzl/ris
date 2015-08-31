#pragma once

#include <memory>
#include <string>
#include <czmq.h>

#include "zmqx/zdispatcher.h"

#include "ris/tracker/trackerctx.h"
#include "ris/tracker/trackertable.h"
#include "ris/tracker/subscriber.h"
#include "ris/snapshot/snapshotservice.h"
#include "ris/tracker/fromregionfactory.h"


class RITrackerActor {
public:
	RITrackerActor();
	~RITrackerActor();

	int start(const std::shared_ptr<TrackerCtx>& ctx);
	void stop();

	inline std::shared_ptr<TrackerCtx> getCtx() const {
		return m_ctx;
	}

private:
	void run(zsock_t* pipe);
	int onPipeReadable(zsock_t* pipe);
	void onFactoryDone(int err,const std::shared_ptr<TrackerFactoryProduct>& product);

	static void actorRunner(zsock_t* pipe,void* args);
	static int pipeReadableAdapter(zloop_t* loop,zsock_t* reader,void* arg);


	void defaultOpt(const std::shared_ptr<google::protobuf::Message>& msg,int err);

private:
	bool					m_running;
	zactor_t*				m_actor;
	zloop_t*				m_loop;
	zsock_t*				m_rep;

	std::shared_ptr<TrackerCtx>				m_ctx;
	std::shared_ptr<RITrackerTable>			m_table;
	std::shared_ptr<RISubscriber>			m_sub;
	std::shared_ptr<SnapshotService>		m_ssvc;
	std::shared_ptr<Dispatcher>				m_disp;
	std::shared_ptr<FromRegionFactory>		m_factory;
};

