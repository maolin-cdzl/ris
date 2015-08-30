#include <glog/logging.h>
#include "ris/tracker/trackertable.h"

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
	m_tid(-1)
{
}

RITrackerTable::~RITrackerTable() {
	stop();
}


int RITrackerTable::start() {
	if( -1 != m_tid ) {
		return -1;
	}

	m_tid = zloop_timer(m_loop,20,0,checkTimerAdapter,this);
	if( m_tid != -1 )
		return 0;
	else
		return -1;
}

void RITrackerTable::stop() {
	if( m_tid != -1 ) {
		zloop_timer_end(m_loop,m_tid);
		m_tid = -1;
	}
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

std::shared_ptr<Region> RITrackerTable::getRegion(const uuid_t& id)  const {
	auto riit = m_regions_index.find(id);
	if( riit != m_regions_index.end() ) {
		return *(riit->second);
	}
	return nullptr;
}

// get region carry this payload
std::shared_ptr<Region> RITrackerTable::routePayload(const uuid_t& id)  const {
	auto piit = m_payloads_index.find(id);
	if( m_payloads_index.end() != piit ) {
		return piit->second->region;
	}
	return nullptr;
}

// get region provide this service,this is round-robin
std::shared_ptr<Region> RITrackerTable::RobinRouteService(const std::string& svc) {
	auto siit = m_services_index.find(svc);
	if( siit != m_services_index.end() ) {
		auto& l = siit->second;
		if( ! l.empty() ) {
			auto regptr = l.front()->region;
			l.splice(l.end(),l,l.begin());		// move first to end
			return std::move(regptr);
		}
	}
	return nullptr;
}

int RITrackerTable::checkTimerAdapter(zloop_t *loop, int timer_id, void *arg) {
	RITrackerTable* self = (RITrackerTable*) arg;
	self->onCheckTimer();
	return 0;
}

// method from IRIObserver
void RITrackerTable::onRegion(const Region& reg) {
	LOG(INFO) << "Recv region: " << reg.id << " pub";
	doAddRegion(reg);
}

void RITrackerTable::onRmRegion(const uuid_t& reg) {
	LOG(INFO) << "Recv remove region: " << reg << " pub";
	doRmRegion(reg);
}

void RITrackerTable::onService(const uuid_t& reg,uint32_t version,const Service& svc) {
	int result = doAddService(reg,svc);
	if( result >= 0 ) {
		updateRegionVersion(reg,version);
	}
}

void RITrackerTable::onRmService(const uuid_t& reg,uint32_t version,const std::string& svc) {
	int result = doRmService(reg,svc);
	if( result >= 0 ) {
		updateRegionVersion(reg,version);
	}
}

void RITrackerTable::onPayload(const uuid_t& reg,uint32_t version,const Payload& pl) {
	int result = doAddPayload(reg,pl);
	if( result >= 0 ) {
		updateRegionVersion(reg,version);
	}
}

void RITrackerTable::onRmPayload(const uuid_t& reg,uint32_t version,const uuid_t& pl) {
	int result = doRmPayload(reg,pl);
	if( result >= 0 ) {
		updateRegionVersion(reg,version);
	}
}

// method from ISnapshotable
snapshot_package_t RITrackerTable::buildSnapshot() {
	snapshot_package_t package;
	package.push_back(std::shared_ptr<snapshot::SnapshotBegin>(new snapshot::SnapshotBegin()));

	std::unordered_map<uuid_t,std::list<Payload>> regionpayloads;
	std::unordered_map<uuid_t,std::list<Service>> regionservices;

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

// method from ISnapshotBuilder
int RITrackerTable::addRegion(const Region& region) {
	onRegion(region);
	return 0;
}

int RITrackerTable::addService(const uuid_t& reg,const Service& svc) {
	int result = doAddService(reg,svc);
	if( result >= 0 )
		return 0;
	else
		return -1;
}

int RITrackerTable::addPayload(const uuid_t& reg,const Payload& pl) {
	int result = doAddPayload(reg,pl);
	if( result >= 0 )
		return 0;
	else
		return -1;
}


void RITrackerTable::updateRegionVersion(std::shared_ptr<Region>& region,uint32_t version) {
	if( region->version != version && region->version + 1 != version ) {
		LOG(WARNING) << "Region: " << region->id << " version jump from " << region->version << " to " << version;
	}
	region->version = version;
}

void RITrackerTable::updateRegionVersion(const uuid_t& region,uint32_t version) {
	auto riit = m_regions_index.find(region);
	if( riit != m_regions_index.end() ) {
		auto rcit = riit->second;
		updateRegionVersion(*rcit,version);
	} else {
		LOG(ERROR) << "Try to update version of unexists region: " << region;
	}
}

std::list<RITrackerTable::service_iterator_t>::iterator RITrackerTable::findRegionService(std::list<RITrackerTable::service_iterator_t>& l,const uuid_t& region) {
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

int RITrackerTable::doAddRegion(const Region& region) {
	int result = 0;
	auto riit = m_regions_index.find(region.id);
	if( riit == m_regions_index.end() ) {
		auto iit = m_regions.insert(m_regions.end(),std::make_shared<Region>(region));
		m_regions_index.insert( std::make_pair(region.id,iit) );
		result = TRACKER_INSERT_NEW;
		LOG(INFO) << "Add new region: " << region.id;
	} else {
		auto rcit = riit->second;
		*(*rcit) = region;
		m_regions.splice(m_regions.end(),m_regions,rcit);	// move latest updated to end
		result = TRACKER_UPDATED;
	}
	return result;
}

int RITrackerTable::doRmRegion(const uuid_t& region) {
	int result = -1;
	auto riit = m_regions_index.find(region);
	if( riit != m_regions_index.end() ) {
		auto rcit = riit->second;
		auto regptr = *rcit;
		m_regions_index.erase(riit);
		m_regions.erase(rcit);

		for(auto scit=m_services.begin(); scit != m_services.end(); ) {
			if( scit->region == regptr ) {
				auto siit = m_services_index.find(scit->service.name);
				assert(siit != m_services_index.end());
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
				assert(piit != m_payloads_index.end());
				m_payloads_index.erase(piit);
				pcit = m_payloads.erase(pcit);
			} else {
				++pcit;
			}
		}
		result = TRACKER_UPDATED;
		LOG(INFO) << "Remove region: " << region;
	} else {
		result = TRACKER_REGION_UNEXISTS;
		LOG(WARNING) << "Try to remove unexists region: " << region;
	}
	return result;
}

int RITrackerTable::doAddService(const uuid_t& region,const Service& svc) {
	int result = -1;
	auto it = m_regions_index.find(region);
	if( it != m_regions_index.end() ) {
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
			result = TRACKER_INSERT_NEW;
			LOG(INFO) << "Add full new service: " << svc.name << " to region: " << region;
		} else {
			std::list<service_iterator_t>& l = siit->second;
			auto lit = findRegionService(l,region);
			if( lit != l.end() ) {
				// update the service info of this region
				(*lit)->service = svc;
				auto scit = (*lit);
				m_services.splice(m_services.end(),m_services,scit); // move latest updated to end
				result = TRACKER_UPDATED;
			} else {
				// if this region has no this service yet.
				RegionService rs;
				rs.region = (*regit);
				rs.service = svc;
				auto scit = m_services.insert(m_services.end(),rs);
				l.push_back(scit);
				result = TRACKER_INSERT_NEW;
				
				LOG(INFO) << "Add service: " << svc.name << " to region: " << region;
			}
		}
	} else {
		result = TRACKER_REGION_UNEXISTS;
		LOG(WARNING) << "Add service: " << svc.name << " to unexists region: " << region;
	}
	return result;
}

int RITrackerTable::doRmService(const uuid_t& region,const std::string& svc) {
	int result = -1;
	auto it = m_regions_index.find(region);
	if( it != m_regions_index.end() ) {
		auto siit = m_services_index.find(svc);
		if( siit == m_services_index.end() ) {
			// if never have this service before.
			result = TRACKER_SERVICE_UNEXISTS;
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
				result = TRACKER_UPDATED;
			} else {
				// this region did not provide this service
				result = TRACKER_SERVICE_UNEXISTS;
				LOG(WARNING) << "Try to remove unexists service: " << svc << " from region: " << region;
			}
		}
	} else {
		result = TRACKER_REGION_UNEXISTS;
		LOG(WARNING) << "Recv rm service: " << svc << " pub to unexists region: " << region;
	}
	return result;
}

int RITrackerTable::doAddPayload(const uuid_t& region,const Payload& pl) {
	int result = -1;
	auto it = m_regions_index.find(region);
	if( it != m_regions_index.end() ) {
		auto regit = it->second;

		auto piit = m_payloads_index.find(pl.id);
		if( piit == m_payloads_index.end() ) {
			// new payload
			RegionPayload rp;
			rp.region = *regit;
			rp.payload = pl;

			auto pcit = m_payloads.insert(m_payloads.end(),rp);
			m_payloads_index.insert( std::make_pair(pl.id,pcit) );
			
			result = TRACKER_INSERT_NEW;
			LOG(INFO) << "New payload: " << pl.id << " to region: " << region;
		} else {
			// update payload info
			auto pcit = piit->second;
			auto regptr = pcit->region;
			if( regptr != *regit ) {
				LOG(ERROR) << "Region: " << region << " replace region: " << regptr->id << " with payload: " << pl.id;
				pcit->region = *regit;
			}
			pcit->payload = pl;
			m_payloads.splice(m_payloads.end(),m_payloads,pcit);	// move latest updated to end
			result = TRACKER_UPDATED;
		}
	} else {
		result = TRACKER_REGION_UNEXISTS;
		LOG(WARNING) << "Try to remove payload: " << pl.id << " from unexists region: " << region;
	}
	return result;
}

int RITrackerTable::doRmPayload(const uuid_t& region,const uuid_t& pl) {
	int result = -1;
	auto it = m_regions_index.find(region);
	if( it != m_regions_index.end() ) {
		auto regit = it->second;

		auto piit = m_payloads_index.find(pl);
		if( piit == m_payloads_index.end() ) {
			result = TRACKER_PAYLOAD_UNEXISTS;
			LOG(WARNING) << "Try to remove unexists payload: " << region << " from region: " << region;
		} else {
			auto pcit = piit->second;
			auto regptr = pcit->region;
			if( regptr == *regit ) {
				m_payloads_index.erase(piit);
				m_payloads.erase(pcit);
				result = TRACKER_UPDATED;
				LOG(INFO) << "Payload: " << pl << " offline from region: " << region;
			} else {
				result = TRACKER_REGION_MISMATCH;
				LOG(ERROR) << "Recv rm payload: " << pl << " pub from region: " << region << " , but it belong to region: " << regptr->id;
			}
		}
	} else {
		result = TRACKER_REGION_UNEXISTS;
		LOG(WARNING) << "Try to remove payload: " << pl << " from unexists region: " << region;
	}
	return result;
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
