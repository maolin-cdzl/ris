#pragma once

#include "ris/ritypes.h"
#include "ris/snapshot/snapshotbuilder.h"
#include "ris/snapshot/snapshotclient.h"
#include "ris/tracker/trackertable.h"
#include "ris/tracker/subscriber.h"

class TrackerFactoryProduct {
public:
	std::shared_ptr<RITrackerTable>				table;
	std::shared_ptr<RISubscriber>				sub;
};

class FromRegionFactory {
public:
	FromRegionFactory(zloop_t* loop);
	~FromRegionFactory();

	int start(const std::string& pub_address);
	void stop();

private:
	zloop_t*						m_loop;
	TrackerFactoryProduct			m_product;

	std::unordered_map<uuid_t
};

