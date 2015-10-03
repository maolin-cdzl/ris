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
			ctx->pub_address = region["pub_address"].c_str();

			// endpoint bus, for busbroker connect
			if( ! region.lookupValue("bus_address",ctx->bus_address) ) {
				ctx->bus_address = "tcp://*.6500";
			}
			if( ! region.lookupValue("bus_identity",ctx->bus_identity) ) {
				ctx->bus_identity = ctx->uuid + "-bus";
			}

			// endpoint snapshot, for snapshot service
			if( ! region.lookupValue("snapshot_address",ctx->snapshot_address) ) {
				ctx->snapshot_address = "tcp://*.6502";
			}
			if( ! region.lookupValue("snapshot_identity",ctx->snapshot_identity) ) {
				ctx->snapshot_identity = ctx->uuid + "-snapshot";
			}

			// endpoint api, for client publish carries
			if( ! region.lookupValue("api_identity",ctx->api_identity) ) {
				ctx->api_identity = ctx->uuid + "-api";
			}
			if( ! region.lookupValue("api_address",ctx->api_address) ) {
				ctx->api_address = "inproc://" + ctx->api_identity;
			}

			// endpoint busapi, for client worker pick message
			if( ! region.lookupValue("bus_api_identity",ctx->bus_api_identity) ) {
				ctx->bus_api_identity = ctx->uuid + "-busapi";
			}
			if( ! region.lookupValue("bus_api_address",ctx->bus_api_address) ) {
				ctx->bus_api_address = "inproc://" + ctx->bus_api_identity;
			}

			// high water mark
			if( ! region.lookupValue("bus_hwm",(unsigned long long&)ctx->bus_hwm) ) {
				ctx->bus_hwm = 5000;
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


