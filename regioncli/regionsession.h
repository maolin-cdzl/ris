#pragma once

#include <string>
#include <czmq.h>

class RegionSession {
public:
	RegionSession();
	~RegionSession();

	inline uint64_t timeout() const {
		return m_timeout;
	}
	inline void timeout(uint64_t tv) {
		m_timeout = tv;
	}

	int connect(const std::string& api_address,uint32_t* version=nullptr);
	void disconnect();

	int newPayload(const std::string& uuid,uint32_t* version=nullptr);
	int rmPayload(const std::string& uuid,uint32_t* version=nullptr);
	int newService(const std::string& name,const std::string& address,uint32_t* version=nullptr);
	int rmService(const std::string& name,uint32_t* version=nullptr);

	int asyncNewPayload(const std::string& uuid);
	int asyncRmPayload(const std::string& uuid);
	int asyncNewService(const std::string& name,const std::string& address);
	int asyncRmService(const std::string& name);

private:
	zsock_t*			m_req;
	uint64_t			m_timeout;
};

