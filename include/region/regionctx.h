#pragma once

#include <string>
#include <memory>

class RegionCtx {
public:
	std::string					uuid;
	std::string					idc;
	std::string					api_address;
	std::string					pub_address;					
	bool						bind_pub;
	std::string					bus_address;
	std::string					snapshot_svc_address;
	std::string					snapshot_worker_address;

	RegionCtx() = default;

	static std::shared_ptr<RegionCtx> loadFile(const std::string& file);
	static std::shared_ptr<RegionCtx> loadStr(const std::string& str);
};

