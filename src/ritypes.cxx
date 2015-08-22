#include <time.h>
#include "ris/ritypes.h"


ri_time_t ri_time_now() {
	ri_time_t now;
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC,&ts);

	now = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
	return now;
}


std::shared_ptr<SnapshotPartition> RegionRt::toSnapshot() {
	std::shared_ptr<SnapshotPartition> part(new SnapshotPartition(id,version));
	
	part->addString(idc.c_str());
	part->addString(address.c_str());
	
	return std::move(part);
}

std::shared_ptr<SnapshotItem> ServiceRt::toSnapshot() {
	std::shared_ptr<SnapshotItem> item { new SnapshotItem(id) };
	char ver[32];
	snprintf(ver,sizeof(ver),"%u",version);
	item->addString(ver);
	item->addString("svc");
	item->addString(address.c_str());

	return std::move(item);
}

std::shared_ptr<SnapshotItem> PayloadRt::toSnapshot() {
	std::shared_ptr<SnapshotItem> item { new SnapshotItem(id) };
	char ver[32];
	snprintf(ver,sizeof(ver),"%u",version);
	item->addString(ver);
	item->addString("pld");

	return std::move(item);
}
