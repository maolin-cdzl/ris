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
	
	part->addValue("idc",idc);
	if( ! msg_url.empty() ) {
		part->addValue("msgurl",msg_url);
	}
	if( ! snapshot_url.empty() ) {
		part->addValue("ssurl",snapshot_url);
	}
	
	return std::move(part);
}

std::shared_ptr<SnapshotItem> Service::toSnapshot() {
	std::shared_ptr<SnapshotItem> item { new SnapshotItem("svc",id) };

	item->addValue("url",url);

	return std::move(item);
}

std::shared_ptr<SnapshotItem> Payload::toSnapshot() {
	std::shared_ptr<SnapshotItem> item { new SnapshotItem("pld",id) };

	return std::move(item);
}
