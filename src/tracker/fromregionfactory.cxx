#include <functional>
#include <glog/logging.h>
#include "ris/tracker/fromregionfactory.h"

using namespace std::placeholders;

FromRegionFactory::FromRegionFactory(zloop_t* loop) :
	m_loop(loop),
	m_tid(-1),
	m_tv_timeout(0)
{
}


FromRegionFactory::~FromRegionFactory() {
	stop();
}

int FromRegionFactory::start(const std::string& pub_address,const std::function<void(int,const std::shared_ptr<TrackerFactoryProduct>&)>& ob) {
	assert(! pub_address.empty() );
	assert( ob );

	if( m_product )
		return -1;
	do {
		m_product = std::make_shared<TrackerFactoryProduct>(std::make_shared<RITrackerTable>(m_loop),std::make_shared<RISubscriber>(m_loop));
		m_sub_cacher = std::make_shared<SubCacher>(std::bind(&FromRegionFactory::onNewRegion,this,_1),std::bind(&FromRegionFactory::onRmRegion,this,_1));
		m_ss_cli = std::make_shared<SnapshotClient>(m_loop);

		m_tid = zloop_timer(m_loop,1000,0,timerAdapter,this);
		if( -1 == m_tid ) {
			LOG(FATAL) << "FromRegionFactory start timer failed";
			break;
		}

		if( -1 == m_product->sub->start(pub_address,m_sub_cacher) ) {
			LOG(FATAL) << "FromRegionFactory start subscriber failed";
			break;
		}

		m_tv_timeout = ri_time_now() + 30000;
		m_observer = ob;
		return 0;
	} while(0);

	stop();
	return -1;
}

void FromRegionFactory::stop() {
	if( m_product )
		m_product.reset();
	if( m_sub_cacher ) {
		m_sub_cacher.reset();
	}
	if( m_ss_cli ) {
		m_ss_cli.reset();
	}
	m_shoted_regions.clear();
	m_unshoted_regions.clear();
	if( m_tid != -1 ) {
		zloop_timer_end(m_loop,m_tid);
		m_tid = -1;
	}
	m_tv_timeout = 0;
	m_observer = nullptr;
}

void FromRegionFactory::onSnapshotDone(uuid_t uuid,int err) {
	if( 0 != err ) {
		LOG(ERROR) << "Error when geting snapshot from region: " << uuid;
	} else {
		LOG(INFO) << "Get snapshot done from region: " << uuid;
		for(auto it=m_unshoted_regions.begin(); it != m_unshoted_regions.end(); ++it) {
			if( (*it) == uuid ) {
				m_shoted_regions.insert(it->id);
				m_unshoted_regions.erase(it);
				break;
			}
		}
	}

	m_tv_timeout = ri_time_now() + 30000;
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
	m_product->table->onRmRegion(region);
}

int FromRegionFactory::nextSnapshot() {
	if( ! m_ss_cli->isActive() ) {
		while( ! m_unshoted_regions.empty() ) {
			auto it = m_unshoted_regions.begin();
			if( -1 == m_ss_cli->start(std::bind(&FromRegionFactory::onSnapshotDone,this,it->id,std::placeholders::_1),m_product->table,it->snapshot_address) ) {
				LOG(ERROR) << "Error when try to get snapshot for region: " << it->id << " from: " << it->snapshot_address;
				m_unshoted_regions.erase(it);
			} else {
				LOG(INFO) << "Start to get snapshot for region: " << it->id << " from: " << it->snapshot_address;
				break;
			}
		}
	}
	return -1;
}

std::shared_ptr<TrackerFactoryProduct> FromRegionFactory::product() {
	for(auto it=m_shoted_regions.begin(); it != m_shoted_regions.end(); ++it) {
		auto region = m_product->table->getRegion(*it);
		if( region != nullptr ) {
			m_sub_cacher->present(region->id,region->version,m_product->table);
			LOG(INFO) << "Present region: " << *it;
		} else {
			LOG(FATAL) << "Present but tracker table has no this region: " << *it;
			return nullptr;
		}
	}

	m_product->sub->setObserver(m_product->table);
	return m_product;
}

int FromRegionFactory::onTimer() {
	if( m_unshoted_regions.empty() && ! m_ss_cli->isActive() ) {
		if( ri_time_now() >= m_tv_timeout ) {
			LOG(INFO) << "Factory idle for enough time,product it";
			auto p = product();
			auto ob = m_observer;
			stop();
			ob(0,p);
		}
	}
	return 0;
}

int FromRegionFactory::timerAdapter(zloop_t* loop,int timerid,void* arg) {
	(void)loop;
	(void)timerid;
	FromRegionFactory* self = (FromRegionFactory*) arg;
	return self->onTimer();
}

