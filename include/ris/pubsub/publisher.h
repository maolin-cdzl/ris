#pragma once

#include <vector>
#include <czmq.h>

#include "ris/ritypes.h"

class RIPublisher {
public:
	RIPublisher();
	virtual ~RIPublisher();

	int start(const Region& region,const std::vector<std::string>& brokers);
	int shutdown();


	int pubService(const uuid_t& reg,uint32_t version,const Service& svc);
	int pubRemoveService(const uuid_t& reg,uint32_t version,const uuid_t& svc);
	int pubPayload(const uuid_t& reg,uint32_t version,const Payload& pl);
	int pubRemovePayload(const uuid_t& reg,uint32_t version,const uuid_t& pl);
	
private:
	zactor_t*					m_actor;

private:
	RIPublisher(const RIPublisher&) = delete;
	RIPublisher(const RIPublisher&&) = delete;
	RIPublisher& operator = (const RIPublisher&) = delete;
};
