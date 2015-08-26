#pragma once

#include <unordered_map>
#include <list>
#include <memory>
#include "ris/ritypes.h"
#include "ris/riobserver.h"
#include "ris/snapshot/snapshotable.h"

class RIRegionTable : public ISnapshotable {
public:
	typedef std::list<ServiceRt>							service_list_t;
	typedef typename service_list_t::iterator				service_list_it_t;
	typedef std::unordered_map<uuid_t,service_list_it_t>	service_index_t;

	typedef std::list<PayloadRt>							payload_list_t;
	typedef typename payload_list_t::iterator				payload_list_it_t;
	typedef std::unordered_map<uuid_t,payload_list_it_t>	payload_index_t;

public:
	RIRegionTable(const Region& reg);
	virtual ~RIRegionTable();

	inline const RegionRt& region() const {
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

	int newService(const Service& svc);
	int delService(const uuid_t& svc);
	int newPayload(const Payload& pl);
	int delPayload(const uuid_t& pl);

	service_list_t update_timeouted_service(ri_time_t timeout,size_t maxcount);
	payload_list_t update_timeouted_payload(ri_time_t timeout,size_t maxcount);

	virtual std::shared_ptr<Snapshot> buildSnapshot();
private:
	RegionRt						m_region;
	IRIObserver*					m_observer;

	service_list_t		m_services;
	service_index_t		m_services_idx;
	payload_list_t		m_payloads;
	payload_index_t		m_payloads_idx;
};
