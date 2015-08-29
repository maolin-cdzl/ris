#include <libconfig.h++>
#include <glog/logging.h>
#include "ris/region/regionctx.h"


int RegionCtx::loadConfig(const std::string& file) {
	libconfig::Config cfg;

	try {
		cfg.readFile(file.c_str());
	} catch( const libconfig::FileIOException& e ) {
		LOG(FATAL) << "RIRegionActor can not open config file: " << file;
		return -1;
	} catch( const libconfig::ParseException& e ) {
		LOG(FATAL) << "Parse error at " << e.getFile() << ":" << e.getLine() << " - " << e.getError();
		return -1;
	}

	try {
		const libconfig::Setting& region = cfg.lookup("region");
		uuid = region["id"].c_str();
		idc = region["idc"].c_str();
		api_address = region["api_address"].c_str();
		bus_address = region["bus_address"].c_str();
		pub_address = region["pub_address"].c_str();
	} catch( const libconfig::SettingNotFoundException& e ) {
		LOG(FATAL) << "Can not found Region setting: " << e.what();
		return -1;
	} catch( const libconfig::SettingTypeException& e ) {
		LOG(FATAL) << "Error when parse Region: " << e.what();
		return -1;
	}

	try {
		if( cfg.exists("region.snapshot") ) {
			const libconfig::Setting& snapshot = cfg.lookup("region.snapshot");
			snapshot_svc_address = snapshot["address"].c_str();
			snapshot_worker_address = snapshot["worker_address"].c_str();
		} else {
			snapshot_svc_address.clear();
			snapshot_worker_address.clear();
		}
	} catch( const libconfig::SettingNotFoundException& e ) {
		LOG(FATAL) << "Can not found Snapshot setting: " << e.what();
		return -1;
	} catch( const libconfig::SettingTypeException& e ) {
		LOG(FATAL) << "Error when parse Snapshot: " << e.what();
		return -1;
	}

	return 0;
}


std::shared_ptr<RegionCtx> loadRegionCtx(const std::string& file) {
	auto cfg = std::make_shared<RegionCtx>();
	if( 0 == cfg->loadConfig(file) ) {
		return cfg;
	} else {
		return nullptr;
	}
}


