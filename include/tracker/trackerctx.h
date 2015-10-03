#pragma once

#include <string>
#include <memory>

#include "snapshot/snapshotctx.h"

class TrackerCtx {
public:
	std::string					idc;
	std::string					pub_address;					

	std::string					api_address;
	std::string					api_identity;

	uint64_t					factory_timeout;

	std::shared_ptr<SnapshotCtx>	snapshot;

	TrackerCtx() = default;

	static std::shared_ptr<TrackerCtx> loadFile(const std::string& file);
	static std::shared_ptr<TrackerCtx> loadStr(const std::string& str);
};


