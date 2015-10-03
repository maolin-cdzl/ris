#pragma once

#include <string>
#include <list>
#include <czmq.h>

#include "ris/ritypes.h"

struct RouteInfo {
	std::string		target;
	EndPoint		endpoint;
};

struct RouteInfoStatistics {
	std::list<Region>		regions;
	std::list<std::string>	services;
	size_t					payload_size;
};

class TrackerSession {
public:
	TrackerSession(uint64_t t=1000);
	~TrackerSession();

	inline uint64_t timeout() const {
		return m_timeout;
	}
	inline void timeout(uint64_t t) {
		m_timeout = t;
	}

	int connect(const std::string& api_address);
	void disconnect();

	std::shared_ptr<RouteInfoStatistics> getStatistics();

	std::shared_ptr<Region> getRegion(const std::string& uuid);

	std::shared_ptr<RouteInfo> getServiceRouteInfo(const std::string& svc);

	std::shared_ptr<RouteInfo> getPayloadRouteInfo(const std::string& payload);

	std::list<RouteInfo> getPayloadsRouteInfo(const std::list<std::string>& payloads);
private:
	zsock_t*				m_req;
	uint64_t				m_timeout;
};

