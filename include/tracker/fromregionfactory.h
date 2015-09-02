#pragma once

#include <unordered_set>
#include "ris/ritypes.h"
#include "snapshot/snapshotbuilder.h"
#include "snapshot/snapshotclient.h"
#include "tracker/trackertable.h"
#include "tracker/subscriber.h"
#include "tracker/subcacher.h"

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

	int start(const std::string& pub_address,const std::function<void(int,const std::shared_ptr<TrackerFactoryProduct>&)>& ob);
	void stop();
private:
	void onSnapshotDone(uuid_t uuid,int err);
	void onNewRegion(const Region& region);
	void onRmRegion(const uuid_t& region);

	int nextSnapshot();
	std::shared_ptr<TrackerFactoryProduct> product();
	int onTimer();

	static int timerAdapter(zloop_t* loop,int timerid,void* arg);
private:
	zloop_t*						m_loop;
	std::shared_ptr<TrackerFactoryProduct>			m_product;

	std::shared_ptr<SubCacher>		m_sub_cacher;
	std::shared_ptr<SnapshotClient>	m_ss_cli;

	std::unordered_set<uuid_t>		m_bad_regions;
	std::unordered_set<uuid_t>		m_shoted_regions;
	std::unordered_set<Region>		m_unshoted_regions;

	int								m_tid;
	ri_time_t						m_tv_timeout;
	std::function<void(int,const std::shared_ptr<TrackerFactoryProduct>&)>	m_observer;
};

