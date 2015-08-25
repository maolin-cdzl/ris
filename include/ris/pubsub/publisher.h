#pragma once

#include <list>
#include <czmq.h>

#include "ris/ritypes.h"

class RIPublisher {
public:
	RIPublisher();
	~RIPublisher();

	int start(const std::list<std::string>& brokers);
	int shutdown();


	int pubRegion(const RegionRt& region);
	int pubRemoveRegion(const uuid_t& reg);

	int pubService(const RegionRt& region,const Service& svc);
	int pubRemoveService(const RegionRt& region,const uuid_t& svc);

	int pubPayload(const RegionRt& region,const Payload& pl);
	int pubRemovePayload(const RegionRt& region,const uuid_t& pl);
	
private:
	std::list<std::string>		m_brokers;
	zsock_t*					m_pub;

private:
	RIPublisher(const RIPublisher&) = delete;
	RIPublisher(const RIPublisher&&) = delete;
	RIPublisher& operator = (const RIPublisher&) = delete;
};
