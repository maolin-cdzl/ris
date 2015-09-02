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

	int loadConfig(const std::string& file);
};

std::shared_ptr<TrackerCtx> loadTrackerCtx(const std::string& file);

