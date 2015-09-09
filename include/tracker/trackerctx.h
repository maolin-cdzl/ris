#pragma once

#include <string>
#include <memory>

class TrackerCtx {
public:
	std::string					idc;
	std::string					api_address;
	std::string					pub_address;					
	std::string					snapshot_svc_address;
	std::string					snapshot_worker_address;
	uint64_t					factory_timeout;

	TrackerCtx() = default;

	static std::shared_ptr<TrackerCtx> loadFile(const std::string& file);
	static std::shared_ptr<TrackerCtx> loadStr(const std::string& str);
};


