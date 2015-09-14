#pragma once

#include <string>
#include <czmq.h>

class RegionSession {
public:
	RegionSession();
	~RegionSession();

	int connect(const std::string& api_address,uint64_t timeout=60000);
	void disconnect();

	// if timeout is 0,it will not wait response from region actor
	int newPayload(const std::string& uuid,uint64_t timeout=0);

	int rmPayload(const std::string& uuid,uint64_t timeout=0);

	int newService(const std::string& name,const std::string& address,uint64_t timeout=0);

	int rmService(const std::string& name,uint64_t timeout=0);
private:
	zsock_t*			m_req;
};

