#pragma once

#include <memory>
#include <string>
#include <libconfig.h++>

class SnapshotCtx {
public:
	std::string					address;
	std::string					identity;
	uint32_t					capacity;
	uint32_t					period_count;
	uint32_t					timeout;

	SnapshotCtx();
	SnapshotCtx(const std::string& addr,const std::string& id);

	static std::shared_ptr<SnapshotCtx> load(const libconfig::Setting& ss);
};

