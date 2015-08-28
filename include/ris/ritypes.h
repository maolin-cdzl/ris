#pragma once

#include <string>
#include <czmq.h>

#include "zmqx/zhelper.h"
#include "ris/snapshot/snapshotable.h"

typedef std::string			uuid_t;
typedef uint64_t			ri_time_t;		// msec

#define ri_time_now			time_now

class Region {
public:
	uuid_t					id;
	std::string				idc;
	std::string				msg_address;
	std::string				snapshot_address;
};

class RegionRt : public Region {
public:
	uint32_t				version;

	RegionRt() = default;
	inline RegionRt(const Region& ref,uint32_t ver=0) :
		Region(ref),
		version(ver)
	{
	}

	std::shared_ptr<SnapshotPartition> toSnapshot() const;
	zmsg_t* toPublish() const;
	static zmsg_t* toPublishDel(const uuid_t& id);

	static bool isPublish(zmsg_t* msg);
	static std::shared_ptr<RegionRt> fromPublish(zmsg_t* msg);

	static bool isPublishDel(zmsg_t* msg);
	static int fromPublishDel(zmsg_t* msg,std::string& reg);
};

class Service {
public:
	uuid_t					id;
	std::string				address;

	std::shared_ptr<SnapshotItem> toSnapshot() const;
	zmsg_t* toPublish(const RegionRt& region) const;

	static zmsg_t* toPublishDel(const RegionRt& region,const uuid_t& id);
};

class ServiceRt : public Service {
public:
	ri_time_t				timeval;

	ServiceRt() = default;
	inline ServiceRt(const Service& ref,ri_time_t tv=0) :
		Service(ref),
		timeval(tv)
	{
	}

};

class Payload {
public:
	uuid_t					id;

	std::shared_ptr<SnapshotItem> toSnapshot() const;
	zmsg_t* toPublish(const RegionRt& region) const;
	static zmsg_t* toPublishDel(const RegionRt& region,const uuid_t& id);
};

class PayloadRt : public Payload {
public:
	ri_time_t				timeval;

	PayloadRt() = default;
	PayloadRt(const Payload& ref,ri_time_t tv = 0) :
		Payload(ref),
		timeval(tv)
	{
	}

};



