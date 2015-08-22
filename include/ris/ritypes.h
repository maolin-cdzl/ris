#pragma once

#include <string>
#include <czmq.h>

#include "ris/snapshot/snapshotable.h"

typedef std::string			uuid_t;
typedef uint64_t			ri_time_t;		// msec

ri_time_t ri_time_now();




class Region {
public:
	uuid_t					id;
	std::string				idc;
	std::string				address;
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

	std::shared_ptr<SnapshotPartition> toSnapshot();
};

class Service {
public:
	uuid_t					id;
	std::string				address;
};

class ServiceRt : public Service {
public:
	ri_time_t				timeval;
	uint32_t				version;

	ServiceRt() = default;
	inline ServiceRt(const Service& ref,ri_time_t tv=0,uint32_t ver=0) :
		Service(ref),
		timeval(tv),
		version(ver)
	{
	}

	std::shared_ptr<SnapshotItem> toSnapshot();
};

class Payload {
public:
	uuid_t					id;
};

class PayloadRt : public Payload {
public:
	ri_time_t				timeval;
	uint32_t				version;

	PayloadRt() = default;
	PayloadRt(const Payload& ref,ri_time_t tv = 0,uint32_t ver = 0) :
		Payload(ref),
		timeval(tv),
		version(ver)
	{
	}

	std::shared_ptr<SnapshotItem> toSnapshot();
};



