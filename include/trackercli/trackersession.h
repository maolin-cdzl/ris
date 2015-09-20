#pragma once

#include <string>
#include <list>
#include <czmq.h>

struct RouteInfo {
	std::string		target;
	std::string		region;
	std::string		address;
};

struct RouteInfoStatistics {
	size_t			region_size;
	size_t			service_size;
	size_t			payload_size;
};

struct RegionInfo {
	std::string			uuid;
	std::string			idc;
	std::string			bus_address;
	std::string			snapshot_address;
	uint32_t			version;
};

class TrackerSession {
public:
	TrackerSession();
	~TrackerSession();

	int connect(const std::string& api_address,uint64_t timeout=1000);
	void disconnect();

	int getStatistics(RouteInfoStatistics* statistics);

	int getRegion(RegionInfo* region,const std::string& uuid);

	int getServiceRouteInfo(RouteInfo* ri,const std::string& svc);

	int getPayloadRouteInfo(RouteInfo* ri,const std::string& payload);

	int getPayloadsRouteInfo(std::list<RouteInfo>& ris,const std::list<std::string>& payloads);
private:
	zsock_t*				m_req;
};

