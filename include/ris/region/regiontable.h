#pragma once

#include <unordered_map>
#include <list>
#include <memory>
#include "ris/ritypes.h"
#include "ris/riobserver.h"

class RIRegionTable {
public:
	RIRegionTable(const Region& reg);
	~RIRegionTable();

	void setObserver(const std::shared_ptr<IRIObserver>& ob);
	void unsetObserver();

	uint32_t newService(const Service& svc);
	uint32_t delService(const uuid_t& svc);
	uint32_t newPayload(const Payload& pl);
	uint32_t delPayload(const uuid_t& pl);

	uint32_t newServiceWithTime(const Service& svc,ri_time_t tv);
	uint32_t newPayloadWithTime(const Payload& pl,ri_time_t tv);
private:
	typedef std::list<ServiceRt>							service_list_t;
	typedef typename service_list_t::iterator				service_list_it_t;
	typedef std::unordered_map<uuid_t,service_list_it_t>	service_index_t;

	typedef std::list<PayloadRt>							payload_list_t;
	typedef typename payload_list_t::iterator				payload_list_it_t;
	typedef std::unordered_map<uuid_t,payload_list_it_t>	payload_index_t;

	const Region					m_region;
	std::shared_ptr<IRIObserver>	m_observer;

	uint32_t			m_version;
	service_list_t		m_services;
	service_index_t		m_services_idx;
	payload_list_t		m_payloads;
	payload_index_t		m_payloads_idx;
};
