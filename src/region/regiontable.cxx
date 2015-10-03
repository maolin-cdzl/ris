#include <glog/logging.h>
#include "region/regiontable.h"

static const long RI_PUB_REPEAT_TIMER			= 20;

RIRegionTable::RIRegionTable(const std::shared_ptr<RegionCtx>& ctx,zloop_t* loop) :
	m_loop(loop),
	m_timer_reg(loop),
	m_timer_repeat(loop),
	m_sec_repub_region(1),
	m_sec_repub_service(60),
	m_sec_repub_payload(60)
{
	m_region.id = ctx->uuid;
	m_region.idc = ctx->idc;
	m_region.bus.address = ctx->bus_address;
	m_region.bus.identity = ctx->bus_identity;
	m_region.snapshot.address = ctx->snapshot->address;
	m_region.snapshot.identity = ctx->snapshot->identity;
	m_region.version = 0;
	m_region.timeval = 0;
}

RIRegionTable::~RIRegionTable() {
	stop();
}

int RIRegionTable::addService(const Service& svc) {
	if( !svc.good() ) {
		LOG(ERROR) << "service not good";
		return -1;
	}
	if( m_services_idx.end() == m_services_idx.find(svc.name) ) {
		++m_region.version;
		auto it = m_services.insert(m_services.end(),svc);
		m_services_idx.insert( std::make_pair(svc.name,it) );

		LOG(INFO) << "new service: " << svc.name << "|" << svc.endpoint.address << " version: " << m_region.version;
		if( m_observer != nullptr ) {
			m_observer->onService(m_region.id,m_region.version,svc);
		}
		return 0;
	} else {
		return -1;
	}
}

int RIRegionTable::rmService(const std::string& svc) {
	auto it = m_services_idx.find(svc);
	if( it != m_services_idx.end() ) {
		auto itl = it->second;
		m_services_idx.erase(it);
		m_services.erase(itl);

		++m_region.version;

		LOG(INFO) << "remove service: " << svc  << " version: " << m_region.version;
		if( m_observer != nullptr ) {
			m_observer->onRmService(m_region.id,m_region.version,svc);
		}
		return 0;
	} else {
		return -1;
	}
}

int RIRegionTable::addPayload(const Payload& pl) {
	if( !pl.good() ) {
		LOG(ERROR) << "Payload not good";
		return -1;
	}
	if( m_payloads_idx.end() == m_payloads_idx.find(pl.id) ) {
		++m_region.version;
		auto it = m_payloads.insert(m_payloads.end(),pl);
		m_payloads_idx.insert( std::make_pair(pl.id,it) );

		LOG(INFO) << "new payload: " << pl.id << " version: " << m_region.version;
		if( m_observer != nullptr ) {
			m_observer->onPayload(m_region.id,m_region.version,pl);
		}
		return 0;
	} else {
		return -1;
	}
}

int RIRegionTable::rmPayload(const ri_uuid_t& pl) {
	auto it = m_payloads_idx.find(pl);
	if( it != m_payloads_idx.end() ) {
		auto itl = it->second;
		m_payloads_idx.erase(it);
		m_payloads.erase(itl);

		++m_region.version;

		LOG(INFO) << "remove payload: " << pl << " version: " << m_region.version;
		if( m_observer != nullptr ) {
			m_observer->onRmPayload(m_region.id,m_region.version,pl);
		}
		return 0;
	} else {
		return -1;
	}
}

RIRegionTable::service_list_t RIRegionTable::update_timeouted_service(ri_time_t timeout,size_t maxcount) {
	service_list_t svcs;
	const ri_time_t now = ri_time_now();
	const ri_time_t expired = now - timeout;
	size_t count = 0;

	for( auto it = m_services.begin(); count < maxcount && it != m_services.end(); ++count ) {
		if( it->timeval <= expired ) {
			it->timeval = now;
			auto itv = it;
			++it;

			svcs.push_back(*itv);
			m_services.splice(m_services.end(),m_services,itv);
		} else {
			break;
		}
	}

	return std::move(svcs);
}

RIRegionTable::payload_list_t RIRegionTable::update_timeouted_payload(ri_time_t timeout,size_t maxcount) {
	payload_list_t pls;
	const ri_time_t now = ri_time_now();
	const ri_time_t expired = now - timeout;
	size_t count = 0;

	for( auto it = m_payloads.begin(); count < maxcount && it != m_payloads.end(); ++count) {
		if( it->timeval <= expired ) {
			it->timeval = now;
			auto itv = it;
			++it;

			pls.push_back(*itv);
			m_payloads.splice(m_payloads.end(),m_payloads,itv);
		} else {
			break;
		}
	}

	return std::move(pls);
}

snapshot_package_t RIRegionTable::buildSnapshot() {
	snapshot_package_t package;
	package.push_back(std::shared_ptr<snapshot::SnapshotBegin>(new snapshot::SnapshotBegin()));

	package.push_back(m_region.toSnapshotBegin());
	
	for(auto it=m_services.begin(); it != m_services.end(); ++it) {
		package.push_back(it->toSnapshot());
	}

	for(auto it=m_payloads.begin(); it != m_payloads.end(); ++it) {
		package.push_back(it->toSnapshot());
	}

	package.push_back(m_region.toSnapshotEnd());
	package.push_back(std::make_shared<snapshot::SnapshotEnd>());
	return std::move(package);
}

int RIRegionTable::start(const std::shared_ptr<IRIObserver>& ob) {
	if( m_timer_reg.isActive() || m_timer_repeat.isActive() )
		return -1;

	if( -1 == m_timer_reg.start( m_sec_repub_region * 1000,0,std::bind<int>(&RIRegionTable::pubRegion,this)) ) {
		return -1;
	}
	if( -1 == m_timer_repeat.start(RI_PUB_REPEAT_TIMER,0,std::bind<int>(&RIRegionTable::pubRepeated,this)) ) {
		m_timer_reg.stop();
		return -1;
	}
	m_observer = ob;
	if( m_observer ) {
		m_observer->onRegion(m_region);
	}
	return 0;
}

void RIRegionTable::stop() {
	if( m_timer_reg.isActive() ) {
		CHECK( m_timer_repeat.isActive());
		m_timer_reg.stop();
		m_timer_repeat.stop();
		if( m_observer ) {
			m_observer->onRmRegion(m_region.id);
		}
	}
}

int RIRegionTable::pubRepeated() {
	if( m_observer ) {
		auto svclist = update_timeouted_service(m_sec_repub_service * 1000,100);
		for(auto it=svclist.begin(); it != svclist.end(); ++it) {
			m_observer->onService(m_region.id,m_region.version,*it);
		}

		const size_t maxcount = (payload_size() / (m_sec_repub_payload * 1000 / RI_PUB_REPEAT_TIMER)) + 100;

		auto pldlist = update_timeouted_payload(m_sec_repub_payload * 1000,maxcount);
		for(auto it=pldlist.begin(); it != pldlist.end(); ++it) {
			m_observer->onPayload(m_region.id,m_region.version,*it);
		}

		if( ! svclist.empty() || ! pldlist.empty() ) {
			LOG(INFO) << "Repeated pub " << svclist.size() << " service and " << pldlist.size() << " payload, version: " << m_region.version;
		}
	}
	return 0;
}

int RIRegionTable::pubRegion() {
	if( m_observer ) {
		m_observer->onRegion(m_region);
	}
	return 0;
}



