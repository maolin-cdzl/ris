#pragma once

#include "tracker/regionproduct.h"
#include "tracker/regionsubcacher.h"
#include "snapshot/snapshotclient.h"


class RegionFactory {
public:
	RegionFactory(zloop_t* loop);
	~RegionFactory();

	int start(const std::function<void(const std::shared_ptr<IRegionLRUReader>&)>& completed,const Region& region);
	void stop();

	inline const std::shared_ptr<RegionSubCacher>& observer() const {
		return m_subcacher;
	}

private:
	void onSnapshotCompleted(int err);
private:
	zloop_t*							m_loop;
	ri_uuid_t							m_region_id;
	std::shared_ptr<RegionProduct>		m_product;
	std::shared_ptr<RegionSubCacher>	m_subcacher;
	std::shared_ptr<SnapshotClient>		m_client;
	std::function<void(const std::shared_ptr<IRegionLRUReader>&)>			m_completed;
};


