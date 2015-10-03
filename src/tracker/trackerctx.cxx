#include <glog/logging.h>
#include <libconfig.h++>
#include "tracker/trackerctx.h"
#include "zmqx/zhelper.h"

static std::shared_ptr<TrackerCtx> loadTrackerCtx(libconfig::Config& cfg) {
	do {
		auto ctx = std::make_shared<TrackerCtx>();
		const std::string uuid = new_short_identity();
		try {
			const libconfig::Setting& tracker = cfg.lookup("tracker");
			ctx->idc = tracker["idc"].c_str();
			ctx->pub_address = tracker["pub_address"].c_str();

			if( ! tracker.lookupValue("api_address",ctx->api_address) ) {
				ctx->api_address = "tcp://*:6600";
			}
			if( ! tracker.lookupValue("api_identity",ctx->api_identity) ) {
				ctx->api_identity = "tracker-" + uuid + "-api";
			}

			if( ! tracker.lookupValue("snapshot_address",ctx->snapshot_address) ) {
				ctx->snapshot_address = "tcp://*:6602";
			}
			if( ! tracker.lookupValue("snapshot_identity",ctx->snapshot_identity) ) {
				ctx->snapshot_identity = "tracker-" + uuid + "-snapshot";
			}

			if( ! tracker.lookupValue("factory_timeout",(unsigned long long&)ctx->factory_timeout) ) {
				ctx->factory_timeout = 30000;
			}
		} catch( const libconfig::SettingNotFoundException& e ) {
			LOG(FATAL) << "Can not found Region setting: " << e.what();
			break;
		} catch( const libconfig::SettingTypeException& e ) {
			LOG(FATAL) << "Error when parse Region: " << e.what();
			break;
		}

		return ctx;
	} while(0);

	return nullptr;
}

std::shared_ptr<TrackerCtx> TrackerCtx::loadFile(const std::string& file) {
	do {
		libconfig::Config cfg;
		try {
			cfg.readFile(file.c_str());
		} catch( const libconfig::FileIOException& e ) {
			LOG(FATAL) << "TrackerCtx can not open config file: " << file;
			break;
		} catch( const libconfig::ParseException& e ) {
			LOG(FATAL) << "Parse error at " << e.getFile() << ":" << e.getLine() << " - " << e.getError();
			break;
		}
		return loadTrackerCtx(cfg);
	} while(0);
	return nullptr;
}


std::shared_ptr<TrackerCtx> TrackerCtx::loadStr(const std::string& str) {
	do {
		libconfig::Config cfg;
		try {
			cfg.readString(str.c_str());
		} catch( const libconfig::ParseException& e ) {
			LOG(FATAL) << "Parse error at " << e.getFile() << ":" << e.getLine() << " - " << e.getError();
			break;
		}
		return loadTrackerCtx(cfg);
	} while(0);
	return nullptr;
}
