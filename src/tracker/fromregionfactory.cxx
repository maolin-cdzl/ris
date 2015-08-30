#include <functional>
#include <glog/logging.h>
#include "ris/tracker/fromregionfactory.h"

using namespace std::placeholders;

FromRegionFactory::FromRegionFactory(zloop_t* loop) :
	m_loop(loop),
	m_product(std::make_shared<RITrackerTable>(loop),std::make_shared<RISubscriber>(loop)),
	m_sub_cacher(new SubCacher(std::bind(&FromRegionFactory::onNewRegion,this,_1),std::bind(&FromRegionFactory::onRmRegion,this,_1))),
	m_ss_cli(new SnapshotClient(loop))
{
}


FromRegionFactory::~FromRegionFactory() {
	stop();
}

int FromRegionFactory::start(const std::string& pub_address) {
	do {
		if( -1 == m_product.sub->start(pub_address,m_sub_cacher) ) {
			break;
		}

		m_product.table = std::make_shared<RITrackerTable>(m_loop);
		m_product.sub = std::make_shared<RISubscriber>(m_loop);
	} while(0);

	return -1;
}

void FromRegionFactory::stop() {
}

void FromRegionFactory::onSnapshotDone(uuid_t uuid,int err) {
	if( 0 != err ) {
		LOG(ERROR) << "Factory get snapshot from region: " << uuid;
		return;
	}
	for(auto it=m_unshoted_regions.begin(); it != m_unshoted_regions.end(); ++it) {
		if( (*it) == uuid ) {
			m_shoted_regions.insert(it->id);
			m_unshoted_regions.erase(it);
			break;
		}
	}

	nextSnapshot();
}

void FromRegionFactory::onNewRegion(const Region& region) {
	m_unshoted_regions.insert(region);
	if( ! m_ss_cli->isActive() ) {
		nextSnapshot();
	}
}

void FromRegionFactory::onRmRegion(const uuid_t& region) {
	LOG(INFO) << "Factory remove region: " << region << " when building";

	for(auto it = m_unshoted_regions.begin(); it != m_unshoted_regions.end(); ++it) {
		if( (*it) == region ) {
			m_unshoted_regions.erase(it);
			break;
		}
	}
	auto it = m_shoted_regions.find(region);
	if( it != m_shoted_regions.end() ) {
		m_shoted_regions.erase(it);
	}
	m_product.table->onRmRegion(region);
}

int FromRegionFactory::nextSnapshot() {
	if( ! m_ss_cli->isActive() ) {
		while( ! m_unshoted_regions.empty() ) {
			auto it = m_unshoted_regions.begin();
			if( -1 == m_ss_cli->start(std::bind(&FromRegionFactory::onSnapshotDone,this,it->id,std::placeholders::_1),m_product.table,it->snapshot_address) ) {
				LOG(ERROR) << "Factory try get snapshot for region: " << it->id << " from: " << it->snapshot_address;
				m_unshoted_regions.erase(it);
			} else {
				break;
			}
		}
	}
	return -1;
}

std::shared_ptr<TrackerFactoryProduct> FromRegionFactory::product() {
	for(auto it=m_shoted_regions.begin(); it != m_shoted_regions.end(); ++it) {
		auto region = m_product.table->getRegion(*it);
		if( region != nullptr ) {
			m_sub_cacher->present(region->id,region->version,m_product.table);
		} else {
			LOG(FATAL) << "Present but tracker table has no this region: " << *it;
			return nullptr;
		}
	}

	m_product.sub->setObserver(m_product.table);
	return std::make_shared<TrackerFactoryProduct>(m_product.table,m_product.sub);
}

