#pragma once

#include <czmq.h>
#include <memory>
#include <string>
#include <list>

#include "ris/region/publisher.h"
#include "ris/region/regiontable.h"
#include "ris/snapshot/snapshotservice.h"

#include "ris/regionapi.pb.h"
#include "zmqx/zdispatcher.h"

class RIRegionActor {
public:
	RIRegionActor();
	~RIRegionActor();

	int start(const std::string& conf);
	int stop();

	inline const Region& region() const {
		return m_region;
	}

	inline const std::string& address() const {
		return m_region_address;
	}
private:
	int loadConfig(const std::string& conf);
	void run(zsock_t* pipe);
	int onPipeReadable(zsock_t* pipe);

	static void actorRunner(zsock_t* pipe,void* args);
	static int pipeReadableAdapter(zloop_t* loop,zsock_t* reader,void* arg);


	void defaultOpt(const std::shared_ptr<google::protobuf::Message>& msg,int err);
	void addService(const std::shared_ptr<region::api::AddService>& msg);
	void rmService(const std::shared_ptr<region::api::RmService>& msg);
	void addPayload(const std::shared_ptr<region::api::AddPayload>& msg);
	void rmPayload(const std::shared_ptr<region::api::RmPayload>& msg);

	static void defaultOptAdapter(const std::shared_ptr<google::protobuf::Message>& msg,int err,void* arg);
	static void addServiceAdapter(const std::shared_ptr<google::protobuf::Message>& msg,void* arg);
	static void rmServiceAdapter(const std::shared_ptr<google::protobuf::Message>& msg,void* arg);
	static void addPayloadAdapter(const std::shared_ptr<google::protobuf::Message>& msg,void* arg);
	static void rmPayloadAdapter(const std::shared_ptr<google::protobuf::Message>& msg,void* arg);
private:
	bool						m_running;
	zactor_t*					m_actor;
	zloop_t*					m_loop;
	zsock_t*					m_rep;

	Region						m_region;
	std::string					m_region_address;
	std::string					m_snapshot_worker_address;
	std::string					m_pub_address;					

	std::shared_ptr<RIRegionTable>		m_table;
	std::shared_ptr<RIPublisher>		m_pub;
	std::shared_ptr<SnapshotService>	m_ssvc;
	std::shared_ptr<Dispatcher>			m_disp;
};

