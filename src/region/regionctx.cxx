#include <libconfig.h++>
#include <glog/logging.h>
#include "region/regionctx.h"

static std::shared_ptr<RegionCtx> loadRegionCtx(libconfig::Config& cfg) {
	do {
		auto ctx = std::make_shared<RegionCtx>();
		try {
			const libconfig::Setting& region = cfg.lookup("region");
			ctx->uuid = region["id"].c_str();
			ctx->idc = region["idc"].c_str();
			ctx->api_address = region["api_address"].c_str();
			ctx->bus_address = region["bus_address"].c_str();
			ctx->pub_address = region["pub_address"].c_str();
			if( region.exists("bind_pub") ) {
				ctx->bind_pub = region["bind_pub"];
			} else {
				ctx->bind_pub = false;
			}
		} catch( const libconfig::SettingNotFoundException& e ) {
			LOG(FATAL) << "Can not found Region setting: " << e.what();
			break;
		} catch( const libconfig::SettingTypeException& e ) {
			LOG(FATAL) << "Error when parse Region: " << e.what();
			break;
		}

		try {
			if( cfg.exists("region.snapshot") ) {
				const libconfig::Setting& snapshot = cfg.lookup("region.snapshot");
				ctx->snapshot_address = snapshot["address"].c_str();
			} else {
				ctx->snapshot_address.clear();
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

std::shared_ptr<RegionCtx> RegionCtx::loadFile(const std::string& file) {
	do {
		libconfig::Config cfg;
		try {
			cfg.readFile(file.c_str());
		} catch( const libconfig::FileIOException& e ) {
			LOG(FATAL) << "RegionCtx can not open config file: " << file;
			break;
		} catch( const libconfig::ParseException& e ) {
			LOG(FATAL) << "Parse error at " << e.getFile() << ":" << e.getLine() << " - " << e.getError();
			break;
		}
		return loadRegionCtx(cfg);
	} while(0);

	return nullptr;
}

std::shared_ptr<RegionCtx> RegionCtx::loadStr(const std::string& str) {
	do {
		libconfig::Config cfg;
		try {
			cfg.readString(str.c_str());
		} catch( const libconfig::ParseException& e ) {
			LOG(FATAL) << "Parse error at " << e.getFile() << ":" << e.getLine() << " - " << e.getError();
			break;
		}
		return loadRegionCtx(cfg);
	} while(0);

	return nullptr;
}


