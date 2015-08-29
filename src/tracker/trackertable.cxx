#include <glog/logging.h>
#include "ris/tracker/trackertable.h"


RITrackerTable::RITrackerTable(zloop_t* loop) :
	m_loop(loop)
{
}

RITrackerTable::~RITrackerTable() {
	stop();
}


int RITrackerTable::start() {
}

void RITrackerTable::stop() {
}

// method from IRIObserver
void RITrackerTable::onRegion(const Region& reg) {
	auto it = m_regions_index.find(reg.id);
	if( it == m_regions_index.end() ) {
		auto iit = m_regions.insert(m_regions.end(),std::make_shared<Region>(reg));
		m_regions_index.insert( std::make_pair(reg.id,iit) );
	} else {
		auto iit = it->second;
		*(*iit) = reg;
	}
}

void RITrackerTable::onRmRegion(const uuid_t& reg) {
	auto riit = m_regions_index.find(reg);
	if( riit != m_regions_index.end() ) {
		LOG(INFO) << "Remove region: " << reg;
		auto rcit = riit->second;
		auto regptr = *rcit;
		m_regions_index.erase(riit);
		m_regions.erase(rcit);

		for(auto scit=m_services.begin(); scit != m_services.end(); ) {
			if( scit->region == regptr ) {
				auto siit = m_services_index.find(scit->service.name);
				assert(siit != m_service_index.end());
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
	} else {
		LOG(WARNING) << "Recv pub to remove unexists region: " << reg;
	}
}

void RITrackerTable::onService(const uuid_t& reg,uint32_t version,const Service& svc) {
	auto it = m_regions_index.find(reg);
	if( it != m_regions_index.end() ) {
		// if region exists
		auto regit = it->second;
		updateRegionVersion(*regit,version);

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

			LOG(INFO) << "Full new service: " << svc.name << " from region: " << reg;
		} else {
			std::list<service_iterator_t>& l = siit->second;
			auto lit = findRegionService(l,reg);
			if( lit != l.end() ) {
				// update the service info of this region
				(*lit)->service = svc;
			} else {
				// if this region has no this service yet.
				RegionService rs;
				rs.region = (*regit);
				rs.service = svc;
				auto scit = m_services.insert(m_services.end(),rs);
				l.push_back(scit);
				
				LOG(INFO) << "New service: " << svc.name << " from region: " << reg;
			}
		}
	} else {
		LOG(WARNING) << "Recv service: " << svc.name << " pub to unexists region: " << reg;
	}
}

void RITrackerTable::onRmService(const uuid_t& reg,uint32_t version,const std::string& svc) {
	auto it = m_regions_index.find(reg);
	if( it != m_regions_index.end() ) {
		auto regit = it->second;
		updateRegionVersion(*regit,version);

		auto siit = m_services_index.find(svc);
		if( siit == m_services_index.end() ) {
			// if never have this service before.
			LOG(WARNING) << "Recv rm service pub from region: " << reg << " to remove service: " << svc << ", which we never know it";
		} else {
			std::list<service_iterator_t>& l = siit->second;
			auto lit = findRegionService(l,reg);
			if( lit != l.end() ) {
				// find this service of region
				auto scit = (*lit);
				l.erase(lit);
				m_services.erase(scit);

				if( l.empty() ) {
					m_services_index.erase(siit);
					LOG(INFO) << "Last service: " << svc << " offline from region: " << reg;
				} else {
					LOG(INFO) << "Service: " << svc << " offline from region: " << reg;
				}
			} else {
				// this region did not provide this service
				LOG(WARNING) << "Recv rm service: " << svc << " pub from region: " << reg << " which not provide it";
			}
		}
	} else {
		LOG(WARNING) << "Recv rm service: " << svc << " pub to unexists region: " << reg;
	}
}

void RITrackerTable::onPayload(const uuid_t& reg,uint32_t version,const Payload& pl) {
	auto it = m_regions_index.find(reg);
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

			LOG(INFO) << "New payload: " << pl.id << " from region: " << reg;
		} else {
			// update payload info
			auto pcit = piit->second;
			auto regptr = pcit->region;
			if( regptr != *regit ) {
				LOG(ERROR) << "Region: " << reg << " replace region: " << regptr->id << " with payload: " << pl.id;
				pcit->region = *regit;
			}
			updateRegionVersion(*regit,version);
			pcit->payload = pl;
		}
	} else {
		LOG(WARNING) << "Recv payload: " << pl.id << " pub to unexists region: " << reg;
	}
}

void RITrackerTable::onRmPayload(const uuid_t& reg,uint32_t version,const uuid_t& pl) {
	auto it = m_regions_index.find(reg);
	if( it != m_regions_index.end() ) {
		auto regit = it->second;

		auto piit = m_payloads_index.find(pl);
		if( piit == m_payloads_index.end() ) {
			LOG(WARNING) << "Recv rm payload pub from region: " << reg << " to remove payload: " << pl << ", which we never know it";
		} else {
			auto pcit = piit->second;
			auto regptr = pcit->region;
			if( regptr == *regit ) {
				updateRegionVersion(*regit,version);

				m_payloads_index.erase(piit);
				m_payloads.erase(pcit);
				LOG(INFO) << "Payload: " << pl << " offline from region: " << reg;
			} else {
				LOG(ERROR) << "Recv rm payload: " << pl << " pub from region: " << reg << " , but it belong to region: " << regptr->id;
			}
		}
	} else {
		LOG(WARNING) << "Recv rm payload: " << pl << " pub to unexists region: " << reg;
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
	int result = -1;
	auto it = m_regions_index.find(reg);
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

			LOG(INFO) << "Add full new service: " << svc.name << " to region: " << reg;
		} else {
			std::list<service_iterator_t>& l = siit->second;
			auto lit = findRegionService(l,reg);
			if( lit != l.end() ) {
				// update the service info of this region
				(*lit)->service = svc;
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
		result = 0;
	} else {
		LOG(WARNING) << "Add service: " << svc.name << " to unexists region: " << reg;
	}
}

int RITrackerTable::addPayload(const uuid_t& reg,const Payload& pl) {
	int result = -1;
	auto it = m_regions_index.find(reg);
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

			LOG(INFO) << "New payload: " << pl.id << " from region: " << reg;
		} else {
			// update payload info
			auto pcit = piit->second;
			auto regptr = pcit->region;
			if( regptr != *regit ) {
				LOG(ERROR) << "Region: " << reg << " replace region: " << regptr->id << " with payload: " << pl.id;
				pcit->region = *regit;
			}
			pcit->payload = pl;
		}
		result = 0;
	} else {
		LOG(WARNING) << "add payload: " << pl.id << " to unexists region: " << reg;
	}
	return result;
}


void RITrackerTable::updateRegionVersion(std::shared_ptr<Region>& region,uint32_t version) {
	if( region->version != version && region->version + 1 != version ) {
		LOG(WARNING) << "Region: " << region->id << " version jump from " << region->version << " to " << version;
	}
	region->version = version;
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

