#pragma once

#include <string>
#include <czmq.h>

class RegionSession {
public:
	RegionSession();
	~RegionSession();

	int connect(const std::string& api_address,uint64_t timeout=60000);
	void disconnect();

	int newPayload(const std::string& uuid);

	int rmPayload(const std::string& uuid);

	int newService(const std::string& name,const std::string& address);

	int rmService(const std::string& name);
private:
	zsock_t*			m_req;
};

