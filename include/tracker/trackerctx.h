#pragma once

#include <string>
#include <memory>

class TrackerCtx {
public:
	std::string					idc;
	std::string					pub_address;					

	std::string					api_address;
	std::string					api_identity;

	std::string					snapshot_address;
	std::string					snapshot_identity;

	uint64_t					factory_timeout;

	TrackerCtx() = default;

	static std::shared_ptr<TrackerCtx> loadFile(const std::string& file);
	static std::shared_ptr<TrackerCtx> loadStr(const std::string& str);
};


