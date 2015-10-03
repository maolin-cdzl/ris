#pragma once

#include <string>
#include <memory>

class RegionCtx {
public:
	std::string					uuid;
	std::string					idc;
	std::string					pub_address;					

	std::string					api_address;
	std::string					api_identity;

	std::string					bus_address;
	std::string					bus_identity;

	std::string					bus_api_address;
	std::string					bus_api_identity;
	size_t						bus_hwm;

	std::string					snapshot_address;
	std::string					snapshot_identity;

	RegionCtx() = default;

	static std::shared_ptr<RegionCtx> loadFile(const std::string& file);
	static std::shared_ptr<RegionCtx> loadStr(const std::string& str);
};

