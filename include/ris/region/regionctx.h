#pragma once

#include <string>
#include <memory>

class RegionCtx {
public:
	std::string					uuid;
	std::string					idc;
	std::string					api_address;
	std::string					pub_address;					
	std::string					bus_address;
	std::string					snapshot_svc_address;
	std::string					snapshot_worker_address;

	RegionCtx() = default;
	int loadConfig(const std::string& file);
};

std::shared_ptr<RegionCtx> loadRegionCtx(const std::string& file);
