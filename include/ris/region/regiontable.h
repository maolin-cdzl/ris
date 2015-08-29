#pragma once

#include <unordered_map>
#include <list>
#include <memory>
#include "ris/ritypes.h"
#include "ris/riobserver.h"
#include "ris/region/regionctx.h"
#include "ris/snapshot/snapshotable.h"

class RIRegionTable : public ISnapshotable {
public:
	typedef std::list<Service>								service_list_t;
	typedef typename service_list_t::iterator				service_list_it_t;
	typedef std::unordered_map<uuid_t,service_list_it_t>	service_index_t;

	typedef std::list<Payload>								payload_list_t;
	typedef typename payload_list_t::iterator				payload_list_it_t;
	typedef std::unordered_map<uuid_t,payload_list_it_t>	payload_index_t;

public:
	RIRegionTable(const std::shared_ptr<RegionCtx>& ctx);
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

	void setObserver(IRIObserver* ob);
	void unsetObserver();

	int addService(const std::string& name,const std::string& address);
	int rmService(const std::string& svc);
	int addPayload(const uuid_t& pl);
	int rmPayload(const uuid_t& pl);

	service_list_t update_timeouted_service(ri_time_t timeout,size_t maxcount);
	payload_list_t update_timeouted_payload(ri_time_t timeout,size_t maxcount);

	virtual snapshot_package_t buildSnapshot();
private:
	Region							m_region;
	IRIObserver*					m_observer;

	service_list_t		m_services;
	service_index_t		m_services_idx;
	payload_list_t		m_payloads;
	payload_index_t		m_payloads_idx;
};
