#include <glog/logging.h>
#include "tracker/trackertable.h"

#define TRACKER_REGION_TIMEOUT_MS				30000
#define TRACKER_SERVICE_TIMEOUT_MS				125000
#define TRACKER_PAYLOAD_TIMEOUT_MS				125000

// results
#define TRACKER_UPDATED								0
#define TRACKER_INSERT_NEW							1
#define TRACKER_ERROR								-1
#define TRACKER_REGION_UNEXISTS						-2
#define TRACKER_REGION_MISMATCH						-3
#define TRACKER_SERVICE_UNEXISTS					-4
#define TRACKER_PAYLOAD_UNEXISTS					-5

RITrackerTable::RITrackerTable(zloop_t* loop) :
	m_loop(loop),
	m_timer(loop)
{
}

RITrackerTable::~RITrackerTable() {
	stop();
}


int RITrackerTable::start() {
	if( m_timer.isActive() ) {
		return -1;
	}
	
	return m_timer.start(20,0,std::bind(&RITrackerTable::onCheckTimer,this));
}

void RITrackerTable::stop() {
	m_timer.stop();
}

// access method
size_t RITrackerTable::region_size() const {
	return m_regions.size();
}

size_t RITrackerTable::service_size() const {
	return m_services.size();
}

size_t RITrackerTable::payload_size() const {
	return m_payloads.size();
}

std::list<std::shared_ptr<Region>> RITrackerTable::regions() const {
	std::list<std::shared_ptr<Region>> r;
	for(auto it = m_regions.begin(); it != m_regions.end(); ++it) {
		r.push_back(*it);
	}
	return std::move(r);
}

std::shared_ptr<Region> RITrackerTable::getRegion(const ri_uuid_t& id)  const {
	auto riit = m_regions_index.find(id);
	if( riit != m_regions_index.end() ) {
		return *(riit->second);
	}
	return nullptr;
}

// get region carry this payload
std::shared_ptr<Region> RITrackerTable::routePayload(const ri_uuid_t& id)  const {
	auto piit = m_payloads_index.find(id);
	if( m_payloads_index.end() != piit ) {
		return piit->second->region;
	}
	return nullptr;
}

// get region provide this service,this is round-robin
std::pair<std::shared_ptr<Region>,std::string> RITrackerTable::robinRouteService(const std::string& svc) {
	std::pair<std::shared_ptr<Region>,std::string> result(nullptr,"");
	auto siit = m_services_index.find(svc);
	if( siit != m_services_index.end() ) {
		auto& l = siit->second;
		if( ! l.empty() ) {
			result.first = l.front()->region;
			result.second = l.front()->service.address;
			l.splice(l.end(),l,l.begin());		// move first to end
		}
	}
	return std::move(result);
}

int RITrackerTable::merge(const std::shared_ptr<IRegionLRUReader>& reader) {
	auto region = reader->getRegion();

	if( m_regions_index.end() != m_regions_index.find(region.id) ) {
		LOG(ERROR) << "Merged region already exists: " << region.id;
		return -1;
	}

	// insert region
	m_regions.push_back(std::make_shared<Region>(region));
	auto regit = std::prev(m_regions.end());
	m_regions_index.insert( std::make_pair(region.id,regit) );

	// insert service
	auto services = reader->getServicesOrderByLRU();
	for(auto it=services.begin(); it != services.end(); ++it) {
		const Service& svc = *it;
		auto siit = m_services_index.find(svc.name);
		if( siit == m_services_index.end() ) {
			// if never have this service before,create new.
			RegionService rs;
			rs.region = (*regit);
			rs.service = svc;

			auto scit = m_services.insert(m_services.end(),rs);
			std::list<service_iterator_t> l;
			l.push_back(scit);
			m_services_index.insert( std::make_pair(svc.name,l) );
			LOG(INFO) << "Add full new service: " << svc.name << " to region: " << region.id;
		} else {
			std::list<service_iterator_t>& l = siit->second;
			auto lit = findRegionService(l,region.id);
			if( lit != l.end() ) {
				// update the service info of this region
				(*lit)->service = svc;
				auto scit = (*lit);
				m_services.splice(m_services.end(),m_services,scit); // move latest updated to end
			} else {
				// if this region has no this service yet.
				RegionService rs;
				rs.region = (*regit);
				rs.service = svc;
				auto scit = m_services.insert(m_services.end(),rs);
				l.push_back(scit);
				
				LOG(INFO) << "Add service: " << svc.name << " to region: " << region.id;
			}
		}
	}

	auto payloads = reader->getPayloadsOrderByLRU();
	for( auto it = payloads.begin(); it != payloads.end(); ++it ) {
		const Payload& pl = *it;
		auto piit = m_payloads_index.find(pl.id);
		if( piit == m_payloads_index.end() ) {
			// new payload
			RegionPayload rp;
			rp.region = *regit;
			rp.payload = pl;

			auto pcit = m_payloads.insert(m_payloads.end(),rp);
			m_payloads_index.insert( std::make_pair(pl.id,pcit) );
			
			LOG(INFO) << "New payload: " << pl.id << " to region: " << region.id;
		} else {
			// update payload info
			auto pcit = piit->second;
			auto regptr = pcit->region;
			if( regptr != *regit ) {
				LOG(WARNING) << "Region: " << region.id << " replace region: " << regptr->id << " with payload: " << pl.id;
				pcit->region = *regit;
			}
			pcit->payload = pl;
			m_payloads.splice(m_payloads.end(),m_payloads,pcit);	// move latest updated to end
		}
	}

	return 0;
}



// method from IRIObserver
void RITrackerTable::onRegion(const Region& reg) {
	auto riit = m_regions_index.find(reg.id);
	if( riit == m_regions_index.end() ) {
		DLOG(INFO) << "Repost onRegion(" << reg.id << ") to next handler";
		if( nextHandler() ) {
			nextHandler()->onRegion(reg);
		}
	} else {
		LOG(INFO) << "Update region: " << reg.id;
		auto rcit = riit->second;
		*(*rcit) = reg;
		m_regions.splice(m_regions.end(),m_regions,rcit);	// move latest updated to end
	}
}

void RITrackerTable::onRmRegion(const ri_uuid_t& reg) {
	if( m_regions_index.end() == m_regions_index.find(reg) ) {
		DLOG(INFO) << "Repost onRmRegion(" << reg << ") to next handler";
		if( nextHandler() ) {
			nextHandler()->onRmRegion(reg);
		}
	} else {
		doRmRegion(reg);
	}
}

void RITrackerTable::onService(const ri_uuid_t& reg,uint32_t version,const Service& svc) {
	auto it = m_regions_index.find(reg);
	if( it == m_regions_index.end() ) {
		DLOG(INFO) << "Repost onService(" << reg << "," << version << "," << svc.name << ") to next handler";
		if( nextHandler() ) {
			nextHandler()->onService(reg,version,svc);
		}
	} else {
		// if region exists
		auto regit = it->second;

		auto siit = m_services_index.find(svc.name);
		if( siit == m_services_index.end() ) {
			// if never have this service before,create new.
			RegionService rs;
			rs.region = (*regit);
			rs.service = svc;

			auto scit = m_services.insert(m_services.end(),rs);
			std::list<service_iterator_t> l;
			l.push_back(scit);
			m_services_index.insert( std::make_pair(svc.name,l) );
			LOG(INFO) << "Add full new service: " << svc.name << " to region: " << reg;
		} else {
			std::list<service_iterator_t>& l = siit->second;
			auto lit = findRegionService(l,reg);
			if( lit != l.end() ) {
				// update the service info of this region
				(*lit)->service = svc;
				auto scit = (*lit);
				m_services.splice(m_services.end(),m_services,scit); // move latest updated to end
			} else {
				// if this region has no this service yet.
				RegionService rs;
				rs.region = (*regit);
				rs.service = svc;
				auto scit = m_services.insert(m_services.end(),rs);
				l.push_back(scit);
				
				LOG(INFO) << "Add service: " << svc.name << " to region: " << reg;
			}
		}

		updateRegionVersion(reg,version);
	}
}

void RITrackerTable::onRmService(const ri_uuid_t& reg,uint32_t version,const std::string& svc) {
	if( m_regions_index.end() == m_regions_index.find(reg) ) {
		DLOG(INFO) << "Repost onRmService(" << reg << "," << version << "," << svc << ") to next handler";
		if( nextHandler() ) {
			nextHandler()->onRmService(reg,version,svc);
		}
	} else {
		doRmService(reg,svc);
		updateRegionVersion(reg,version);
	}
}

void RITrackerTable::onPayload(const ri_uuid_t& reg,uint32_t version,const Payload& pl) {
	auto it = m_regions_index.find(reg);
	if( it == m_regions_index.end() ) {
		DLOG(INFO) << "Repost onPayload(" << reg << "," << version << "," << pl.id << ") to next handler";
		if( nextHandler() ) {
			nextHandler()->onPayload(reg,version,pl);
		}
	} else {
		auto regit = it->second;

		auto piit = m_payloads_index.find(pl.id);
		if( piit == m_payloads_index.end() ) {
			// new payload
			RegionPayload rp;
			rp.region = *regit;
			rp.payload = pl;

			auto pcit = m_payloads.insert(m_payloads.end(),rp);
			m_payloads_index.insert( std::make_pair(pl.id,pcit) );
			
			LOG(INFO) << "New payload: " << pl.id << " to region: " << reg;
		} else {
			// update payload info
			auto pcit = piit->second;
			auto regptr = pcit->region;
			if( regptr != *regit ) {
				LOG(WARNING) << "Region: " << reg << " replace region: " << regptr->id << " with payload: " << pl.id;
				pcit->region = *regit;
			}
			pcit->payload = pl;
			m_payloads.splice(m_payloads.end(),m_payloads,pcit);	// move latest updated to end
		}

		updateRegionVersion(reg,version);
	}
}

void RITrackerTable::onRmPayload(const ri_uuid_t& reg,uint32_t version,const ri_uuid_t& pl) {
	if( m_regions_index.end() == m_regions_index.find(reg) ) {
		DLOG(INFO) << "Repost onRmPayload(" << reg << "," << version << "," << pl << ") to next handler";
		if( nextHandler() ) {
			nextHandler()->onRmPayload(reg,version,pl);
		}
	} else {
		doRmPayload(reg,pl);
		updateRegionVersion(reg,version);
	}
}

// method from ISnapshotable
snapshot_package_t RITrackerTable::buildSnapshot() {
	snapshot_package_t package;
	package.push_back(std::shared_ptr<snapshot::SnapshotBegin>(new snapshot::SnapshotBegin()));

	std::unordered_map<ri_uuid_t,std::list<Payload>> regionpayloads;
	std::unordered_map<ri_uuid_t,std::list<Service>> regionservices;

	for(auto it=m_services.begin(); it != m_services.end(); ++it) {
		regionservices[ it->region->id ].push_back(it->service);
	}
	for(auto it=m_payloads.begin(); it != m_payloads.end(); ++it) {
		regionpayloads[ it->region->id ].push_back(it->payload);
	}

	for(auto rcit=m_regions.begin(); rcit != m_regions.end(); ++rcit) {
		package.push_back((*rcit)->toSnapshotBegin());

		const std::list<Service>& svcs = regionservices[ (*rcit)->id ];
		for(auto it=svcs.begin(); it != svcs.end(); ++it) {
			package.push_back(it->toSnapshot());
		}

		const std::list<Payload>& payloads = regionpayloads[ (*rcit)->id ];
		for(auto it=payloads.begin(); it != payloads.end(); ++it) {
			package.push_back(it->toSnapshot());
		}
		package.push_back((*rcit)->toSnapshotEnd());
	}

	package.push_back(std::shared_ptr<snapshot::SnapshotEnd>(new snapshot::SnapshotEnd()));
	return std::move(package);
}

void RITrackerTable::updateRegionVersion(std::shared_ptr<Region>& region,uint32_t version) {
	if( region->version != version && region->version + 1 != version ) {
		LOG(WARNING) << "Region: " << region->id << " version jump from " << region->version << " to " << version;
	}
	region->version = version;
}

void RITrackerTable::updateRegionVersion(const ri_uuid_t& region,uint32_t version) {
	auto riit = m_regions_index.find(region);
	if( riit != m_regions_index.end() ) {
		auto rcit = riit->second;
		updateRegionVersion(*rcit,version);
	} else {
		LOG(ERROR) << "Try to update version of unexists region: " << region;
	}
}

std::list<RITrackerTable::service_iterator_t>::iterator RITrackerTable::findRegionService(std::list<RITrackerTable::service_iterator_t>& l,const ri_uuid_t& region) {
	for(auto it = l.begin(); it != l.end(); ) {
		auto regptr = (*it)->region;
		if( regptr == nullptr ) {
			// if the region already offline,remove service which belong it
			it = l.erase(it);
		} else {
			if( regptr->id == region ) {
				return it;
			}
			++it;
		}
	}
	return l.end();
}

void RITrackerTable::doRmRegion(const ri_uuid_t& region) {
	auto riit = m_regions_index.find(region);
	CHECK(m_regions_index.end() != riit);

	LOG(INFO) << "Remove region: " << region;
	auto rcit = riit->second;
	auto regptr = *rcit;
	m_regions_index.erase(riit);
	m_regions.erase(rcit);

	for(auto scit=m_services.begin(); scit != m_services.end(); ) {
		if( scit->region == regptr ) {
			auto siit = m_services_index.find(scit->service.name);
			CHECK(m_services_index.end() != siit);
			std::list<service_iterator_t>& l = siit->second;
			for(auto lit = l.begin(); lit != l.end(); ++lit) {
				if( *lit == scit ) {
					l.erase(lit);
					break;
				}
			}
			if( l.empty() ) {
				m_services_index.erase(siit);
			}
			scit = m_services.erase(scit);
		} else {
			++scit;
		}
	}

	for(auto pcit=m_payloads.begin(); pcit != m_payloads.end(); ) {
		if( pcit->region == regptr ) {
			auto piit = m_payloads_index.find(pcit->payload.id);
			CHECK(m_payloads_index.end() != piit);
			m_payloads_index.erase(piit);
			pcit = m_payloads.erase(pcit);
		} else {
			++pcit;
		}
	}
}

void RITrackerTable::doRmService(const ri_uuid_t& region,const std::string& svc) {
	auto siit = m_services_index.find(svc);
	if( siit == m_services_index.end() ) {
		// if never have this service before.
		LOG(WARNING) << "Try to remove unknown service: " << svc << " from region: " << region;
	} else {
		std::list<service_iterator_t>& l = siit->second;
		auto lit = findRegionService(l,region);
		if( lit != l.end() ) {
			// find this service of region
			auto scit = (*lit);
			l.erase(lit);
			m_services.erase(scit);

			if( l.empty() ) {
				m_services_index.erase(siit);
				LOG(INFO) << "Last service: " << svc << " offline from region: " << region;
			} else {
				LOG(INFO) << "Service: " << svc << " offline from region: " << region;
			}
		} else {
			// this region did not provide this service
			LOG(WARNING) << "Try to remove unexists service: " << svc << " from region: " << region;
		}
	}
}

void RITrackerTable::doRmPayload(const ri_uuid_t& region,const ri_uuid_t& pl) {
	auto it = m_regions_index.find(region);
	if( it != m_regions_index.end() ) {
		auto regit = it->second;

		auto piit = m_payloads_index.find(pl);
		if( piit == m_payloads_index.end() ) {
			LOG(WARNING) << "Try to remove unexists payload: " << region << " from region: " << region;
		} else {
			auto pcit = piit->second;
			auto regptr = pcit->region;
			if( regptr == *regit ) {
				m_payloads_index.erase(piit);
				m_payloads.erase(pcit);
				LOG(INFO) << "Payload: " << pl << " offline from region: " << region;
			} else {
				LOG(ERROR) << "Recv rm payload: " << pl << " pub from region: " << region << " , but it belong to region: " << regptr->id;
			}
		}
	} else {
		LOG(WARNING) << "Try to remove payload: " << pl << " from unexists region: " << region;
	}
}

int RITrackerTable::onCheckTimer() {
	size_t limits = 3;
	const ri_time_t now = ri_time_now();
	while( limits != 0 && ! m_regions.empty() ) {
		auto& region = m_regions.front();
		if( region->timeval + TRACKER_REGION_TIMEOUT_MS <= now ) {
			LOG(WARNING) << "Region: " << region->id << " timeouted!";
			doRmRegion(region->id);
			--limits;
		} else {
			break;
		}
	}

	limits = 100;
	while( limits != 0 && !m_services.empty() ) {
		auto& region = m_services.front().region;
		auto& svc = m_services.front().service;
		if( svc.timeval + TRACKER_SERVICE_TIMEOUT_MS <= now ) {
			LOG(WARNING) << "Region: " << region->id << " service: " << svc.name << " timeouted!";
			doRmService(region->id,svc.name);
			--limits;
		} else {
			break;
		}
	}

	limits = 100;
	while( limits != 0 && !m_payloads.empty() ) {
		auto& region = m_payloads.front().region;
		auto& pl = m_payloads.front().payload;
		if( pl.timeval + TRACKER_PAYLOAD_TIMEOUT_MS <= now ) {
			LOG(WARNING) << "Region: " << region->id << " payload: " << pl.id << " timeouted!";
			doRmPayload(region->id,pl.id);
			--limits;
		} else {
			break;
		}
	}
	return 0;
}

