#include <glog/logging.h>
#include "snapshot/snapshotctx.h"
#include "zmqx/zhelper.h"

SnapshotCtx::SnapshotCtx() :
	capacity(4),
	period_count(100),
	timeout(3000)
{
}

SnapshotCtx::SnapshotCtx(const std::string& addr,const std::string& id) :
	address(addr),
	identity(id),
	capacity(4),
	period_count(100),
	timeout(3000)
{
}

std::shared_ptr<SnapshotCtx> SnapshotCtx::load(const libconfig::Setting& ss) {
	try {
		auto ctx = std::make_shared<SnapshotCtx>();
		ctx->address = ss["address"].c_str();
		if( !ss.lookupValue("identity",ctx->identity) ) {
			ctx->identity = new_short_identity() + "-snapshot";
		}
		if( !ss.lookupValue("capacity",ctx->capacity) ) {
			ctx->capacity = 4;
		}
		if( !ss.lookupValue("period_count",ctx->period_count) ) {
			ctx->period_count = 100;
		}
		if( !ss.lookupValue("timeout",ctx->timeout) ) {
			ctx->timeout = 3000;
		}
		return std::move(ctx);
	} catch( const libconfig::SettingNotFoundException& e ) {
		LOG(FATAL) << "Can not found Region setting: " << e.what();
	} catch( const libconfig::SettingTypeException& e ) {
		LOG(FATAL) << "Error when parse Region: " << e.what();
	}

	return nullptr;
}

