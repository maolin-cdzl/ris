#include <time.h>
#include "ris/ritypes.h"


ri_time_t ri_time_now() {
	ri_time_t now;
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC,&ts);

	now = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
	return now;
}


std::shared_ptr<SnapshotPartition> RegionRt::toSnapshot() const {
	std::shared_ptr<SnapshotPartition> part(new SnapshotPartition(id,version));
	
	part->addValue("idc",idc);
	if( ! msg_address.empty() ) {
		part->addValue("msgaddr",msg_address);
	}
	if( ! snapshot_address.empty() ) {
		part->addValue("ssaddr",snapshot_address);
	}
	
	return std::move(part);
}

zmsg_t* RegionRt::toPublish() const {
	char ver[32];
	zmsg_t* msg = zmsg_new();
	snprintf(ver,sizeof(ver),"%u",version);

	zmsg_addstr(msg,"#reg");
	zmsg_addstr(msg,id.c_str());
	zmsg_addstr(msg,ver);
	zmsg_addstr(msg,idc.c_str());
	if( msg_address.empty() ) {
		zmsg_addstr(msg,"none");
	} else {
		zmsg_addstr(msg,msg_address.c_str());
	}
	if( snapshot_address.empty() ) {
		zmsg_addstr(msg,"none");
	} else {
		zmsg_addstr(msg,snapshot_address.c_str());
	}

	return msg;
}

zmsg_t* RegionRt::toPublishDel(const uuid_t& id) {
	zmsg_t* msg = zmsg_new();
	zmsg_addstr(msg,"delreg");
	zmsg_addstr(msg,id.c_str());

	return msg;
}

bool RegionRt::isPublish(zmsg_t* msg) {
	assert(msg);
	if( zmsg_size(msg) >= 6 ) {
		return zframe_streq( zmsg_first(msg),"#reg" );
	} else {
		return false;
	}
}

std::shared_ptr<RegionRt> RegionRt::fromPublish(zmsg_t* msg) {
	std::shared_ptr<RegionRt> reg(new RegionRt);
	char* str = nullptr;
	assert( isPublish(msg) );

	zframe_t* fr = zmsg_first(msg);
	fr = zmsg_next(msg);
	str = zframe_strdup(fr);
	reg->id = str;
	free(str);

	fr = zmsg_next(msg);
	str = zframe_strdup(fr);
	reg->version = (uint32_t) strtoll(str,nullptr,10);
	free(str);

	fr = zmsg_next(msg);
	str = zframe_strdup(fr);
	reg->idc = str;
	free(str);

	fr = zmsg_next(msg);
	str = zframe_strdup(fr);
	reg->idc = str;
	free(str);

	return reg;
}

bool RegionRt::isPublishDel(zmsg_t* msg) {
}

int RegionRt::fromPublishDel(zmsg_t* msg,std::string& reg) {
}

/**
 * class Service 
 */


std::shared_ptr<SnapshotItem> Service::toSnapshot() const {
	std::shared_ptr<SnapshotItem> item { new SnapshotItem("svc",id) };

	item->addValue("addr",address);

	return std::move(item);
}

zmsg_t* Service::toPublish(const RegionRt& region) const {
	char ver[32];
	zmsg_t* msg = zmsg_new();
	snprintf(ver,sizeof(ver),"%u",region.version);

	zmsg_addstr(msg,"#svc");
	zmsg_addstr(msg,region.id.c_str());
	zmsg_addstr(msg,ver);
	zmsg_addstr(msg,id.c_str());
	zmsg_addstr(msg,address.c_str());
	
	return msg;
}

zmsg_t* Service::toPublishDel(const RegionRt& region,const uuid_t& id) {
	char ver[32];
	zmsg_t* msg = zmsg_new();
	snprintf(ver,sizeof(ver),"%u",region.version);

	zmsg_addstr(msg,"delsvc");
	zmsg_addstr(msg,region.id.c_str());
	zmsg_addstr(msg,ver);
	zmsg_addstr(msg,id.c_str());

	return msg;
}


std::shared_ptr<SnapshotItem> Payload::toSnapshot() const {
	std::shared_ptr<SnapshotItem> item { new SnapshotItem("pld",id) };

	return std::move(item);
}

zmsg_t* Payload::toPublish(const RegionRt& region) const {
	char ver[32];
	zmsg_t* msg = zmsg_new();
	snprintf(ver,sizeof(ver),"%u",region.version);

	zmsg_addstr(msg,"#pld");
	zmsg_addstr(msg,region.id.c_str());
	zmsg_addstr(msg,ver);
	zmsg_addstr(msg,id.c_str());
	
	return msg;
}

zmsg_t* Payload::toPublishDel(const RegionRt& region,const uuid_t& id) {
	char ver[32];
	zmsg_t* msg = zmsg_new();
	snprintf(ver,sizeof(ver),"%u",region.version);

	zmsg_addstr(msg,"delpld");
	zmsg_addstr(msg,region.id.c_str());
	zmsg_addstr(msg,ver);
	zmsg_addstr(msg,id.c_str());

	return msg;
}

