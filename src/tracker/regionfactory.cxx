#include <glog/logging.h>
#include "tracker/regionfactory.h"


RegionFactory::RegionFactory(zloop_t* loop) :
	m_loop(loop)
{
}

RegionFactory::~RegionFactory() {
	stop();
}

int RegionFactory::start(const std::function<void(const ri_uuid_t&,const std::shared_ptr<IRegionLRUReader>&)>& completed,const Region& region) {
	if( m_client )
		return -1;

	LOG(INFO) << "RegionFactory try sync to region: " << region.id << "(" << region.version << ")";
	do {
		m_product = std::make_shared<RegionProduct>();
		m_client = std::make_shared<SnapshotClient>(m_loop);
		if( -1 == m_client->start(std::bind(&RegionFactory::onSnapshotCompleted,this,std::placeholders::_1),m_product,region.snapshot_address) ) {
			break;
		}
		m_subcacher = std::make_shared<RegionSubCacher>(region);
		m_region_id = region.id;
		m_completed = completed;
		return 0;
	} while(0);

	stop();
	return -1;
}

void RegionFactory::stop() {
	m_client.reset();
	m_subcacher.reset();
	m_product.reset();
	m_region_id.clear();
	m_completed = nullptr;
}

void RegionFactory::onSnapshotCompleted(int err) {
	std::shared_ptr<IRegionLRUReader> product(nullptr);
	if( 0 == err ) {
		auto& region = m_product->getRegion();
		if( 0 == m_subcacher->present(region.version,m_product) ) {
			product = m_product;
		}
	}

	auto completed = m_completed;
	auto region_id = m_region_id;
	stop();
	completed(region_id,product);
}


