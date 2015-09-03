#pragma once

#include <unordered_map>
#include <list>
#include <memory>
#include "ris/ritypes.h"
#include "ris/riobserver.h"
#include "region/regionctx.h"
#include "snapshot/snapshotable.h"

class RIRegionTable : public ISnapshotable {
public:
	typedef std::list<Service>								service_list_t;
	typedef typename service_list_t::iterator				service_list_it_t;
	typedef std::unordered_map<ri_uuid_t,service_list_it_t>	service_index_t;

	typedef std::list<Payload>								payload_list_t;
	typedef typename payload_list_t::iterator				payload_list_it_t;
	typedef std::unordered_map<ri_uuid_t,payload_list_it_t>	payload_index_t;

public:
	RIRegionTable(const std::shared_ptr<RegionCtx>& ctx,zloop_t* loop);
	virtual ~RIRegionTable();

	inline const Region& region() const {
		return m_region;
	}

	inline uint32_t version() const {
		return m_region.version;
	}

	inline size_t service_size() const {
		return m_services.size();
	}

	inline size_t payload_size() const {
		return m_payloads.size();
	}

	int addService(const std::string& name,const std::string& address);
	int rmService(const std::string& svc);
	int addPayload(const ri_uuid_t& pl);
	int rmPayload(const ri_uuid_t& pl);

	virtual snapshot_package_t buildSnapshot();

	int start(const std::shared_ptr<IRIObserver>& ob);
	void stop();
private:
	service_list_t update_timeouted_service(ri_time_t timeout,size_t maxcount);
	payload_list_t update_timeouted_payload(ri_time_t timeout,size_t maxcount);

	int pubRegion();
	int pubRepeated();
	static int onRegionPubTimer(zloop_t *loop, int timer_id, void *arg);
	static int onRepeatPubTimer(zloop_t *loop, int timer_id, void *arg);
private:
	zloop_t*						m_loop;
	Region							m_region;
	std::shared_ptr<IRIObserver>	m_observer;

	service_list_t		m_services;
	service_index_t		m_services_idx;
	payload_list_t		m_payloads;
	payload_index_t		m_payloads_idx;

	int								m_tid_reg;
	int								m_tid_repeat;
};
