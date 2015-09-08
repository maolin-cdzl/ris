#include <glog/logging.h>
#include <libconfig.h++>
#include "tracker/trackerctx.h"

static std::shared_ptr<TrackerCtx> loadTrackerCtx(libconfig::Config& cfg) {
	do {
		auto ctx = std::make_shared<TrackerCtx>();
		try {
			const libconfig::Setting& tracker = cfg.lookup("tracker");
			ctx->idc = tracker["idc"].c_str();
			ctx->api_address = tracker["api_address"].c_str();
			ctx->pub_address = tracker["pub_address"].c_str();
		} catch( const libconfig::SettingNotFoundException& e ) {
			LOG(FATAL) << "Can not found Region setting: " << e.what();
			break;
		} catch( const libconfig::SettingTypeException& e ) {
			LOG(FATAL) << "Error when parse Region: " << e.what();
			break;
		}

		try {
			if( cfg.exists("tracker.snapshot") ) {
				const libconfig::Setting& snapshot = cfg.lookup("tracker.snapshot");
				ctx->snapshot_svc_address = snapshot["address"].c_str();
				ctx->snapshot_worker_address = snapshot["worker_address"].c_str();
			} else {
				ctx->snapshot_svc_address.clear();
				ctx->snapshot_worker_address.clear();
			}
		} catch( const libconfig::SettingNotFoundException& e ) {
			LOG(FATAL) << "Can not found Snapshot setting: " << e.what();
			break;
		} catch( const libconfig::SettingTypeException& e ) {
			LOG(FATAL) << "Error when parse Snapshot: " << e.what();
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
