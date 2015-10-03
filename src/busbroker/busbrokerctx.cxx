#include <libconfig.h++>
#include <glog/logging.h>
#include "busbroker/busbrokerctx.h"

static std::shared_ptr<BusBrokerCtx> loadBusBrokerCtx(libconfig::Config& cfg) {
	do {
		auto ctx = std::make_shared<BusBrokerCtx>();
		try {
			const libconfig::Setting& busbroker = cfg.lookup("busbroker");
			ctx->frontend_address = busbroker["frontend_address"].c_str();
			ctx->frontend_worker_address = busbroker["frontend_worker_address"].c_str();
			ctx->frontend_reply_address = busbroker["frontend_reply_address"].c_str();
			ctx->backend_address = busbroker["backend_address"].c_str();
			ctx->backend_worker_address = busbroker["backend_worker_address"].c_str();
			ctx->tracker_api_address = busbroker["tracker_api_address"].c_str();
			if( busbroker.exists("worker_count") ) {
				ctx->worker_count = busbroker["worker_count"];
			} else {
				ctx->worker_count = 16;
			}
		} catch( const libconfig::SettingNotFoundException& e ) {
			LOG(FATAL) << "Can not found BusBroker setting: " << e.what();
			break;
		} catch( const libconfig::SettingTypeException& e ) {
			LOG(FATAL) << "Error when parse busbroker: " << e.what();
			break;
		}

		return ctx;
	} while(0);

	return nullptr;
}

std::shared_ptr<BusBrokerCtx> BusBrokerCtx::loadFile(const std::string& file) {
	do {
		libconfig::Config cfg;
		try {
			cfg.readFile(file.c_str());
		} catch( const libconfig::FileIOException& e ) {
			LOG(FATAL) << "BusBrokerCtx can not open config file: " << file;
			break;
		} catch( const libconfig::ParseException& e ) {
			LOG(FATAL) << "Parse error at " << e.getFile() << ":" << e.getLine() << " - " << e.getError();
			break;
		}
		return loadBusBrokerCtx(cfg);
	} while(0);

	return nullptr;
}

std::shared_ptr<BusBrokerCtx> BusBrokerCtx::loadStr(const std::string& str) {
	do {
		libconfig::Config cfg;
		try {
			cfg.readString(str.c_str());
		} catch( const libconfig::ParseException& e ) {
			LOG(FATAL) << "Parse error at " << e.getFile() << ":" << e.getLine() << " - " << e.getError();
			break;
		}
		return loadBusBrokerCtx(cfg);
	} while(0);

	return nullptr;
}

