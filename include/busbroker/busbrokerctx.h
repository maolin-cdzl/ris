#pragma once

#include <string>
#include <memory>

struct BusBrokerCtx {
	std::string					frontend_address;
	std::string					frontend_worker_address;
	std::string					frontend_reply_address;

	std::string					backend_address;
	std::string					backend_worker_address;

	std::string					tracker_api_address;
	size_t						worker_count;

	static std::shared_ptr<BusBrokerCtx> loadFile(const std::string& file);
	static std::shared_ptr<BusBrokerCtx> loadStr(const std::string& str);
};



