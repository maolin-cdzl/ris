#pragma once

#include <unordered_set>
#include "ris/ritypes.h"
#include "ris/snapshot/snapshotbuilder.h"
#include "ris/snapshot/snapshotclient.h"
#include "ris/tracker/trackertable.h"
#include "ris/tracker/subscriber.h"
#include "ris/tracker/subcacher.h"

class TrackerFactoryProduct {
public:
	std::shared_ptr<RITrackerTable>				table;
	std::shared_ptr<RISubscriber>				sub;

	TrackerFactoryProduct() = default;
	inline TrackerFactoryProduct(const std::shared_ptr<RITrackerTable>& t,const std::shared_ptr<RISubscriber>& s) :
		table(t),sub(s)
	{
	}
};

class FromRegionFactory {
public:
	FromRegionFactory(zloop_t* loop);
	~FromRegionFactory();

	int start(const std::string& pub_address);
	void stop();
private:
	void onSnapshotDone(uuid_t uuid,int err);
	void onNewRegion(const Region& region);
	void onRmRegion(const uuid_t& region);

	int nextSnapshot();
	std::shared_ptr<TrackerFactoryProduct> product();
private:
	zloop_t*						m_loop;
	TrackerFactoryProduct			m_product;

	std::shared_ptr<SubCacher>		m_sub_cacher;
	std::shared_ptr<SnapshotClient>	m_ss_cli;

	std::unordered_set<uuid_t>		m_shoted_regions;
	std::unordered_set<Region>		m_unshoted_regions;
};

